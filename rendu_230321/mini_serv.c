/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   mini_serv.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: pwolff <pwolff@student.42mulhouse.fr>>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/03/21 14:16:41 by pwolff            #+#    #+#             */
/*   Updated: 2023/03/21 16:00:47 by pwolff           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <errno.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>


int		clients = 0, fdMax = 0;
fd_set	rfds, wfds, fds;
char	rbuff[65000], wbuff[100];
char	*msg[65536];  //python3  2**16
int		idx[65536];






int extract_message(char **buf, char **msg)
{
	char	*newbuf;
	int	i;

	*msg = 0;
	if (*buf == 0)
		return (0);
	i = 0;
	while ((*buf)[i])
	{
		if ((*buf)[i] == '\n')
		{
			newbuf = calloc(1, sizeof(*newbuf) * (strlen(*buf + i + 1) + 1));
			if (newbuf == 0)
				return (-1);
			strcpy(newbuf, *buf + i + 1);
			*msg = *buf;
			(*msg)[i + 1] = 0;
			*buf = newbuf;
			return (1);
		}
		i++;
	}
	return (0);
}

char *str_join(char *buf, char *add)
{
	char	*newbuf;
	int		len;

	if (buf == 0)
		len = 0;
	else
		len = strlen(buf);
	newbuf = malloc(sizeof(*newbuf) * (len + strlen(add) + 1));
	if (newbuf == 0)
		return (0);
	newbuf[0] = 0;
	if (buf != 0)
		strcat(newbuf, buf);
	free(buf);
	strcat(newbuf, add);
	return (newbuf);
}

void	ft_fatal(void)
{
	write(2, "Fatal error\n", 12);
	exit(1);
}

void	display(int client, char *wbuff)
{
	for (int i = 0; i <= fdMax; i++)
	{
		if (FD_ISSET(i, &wfds) && i != client)
			send(i, wbuff, strlen(wbuff), 0);
	}
}

void	delivre(int i)
{
	char	*s;
	while (extract_message(&(msg[i]), &s))
	{
		sprintf(wbuff, "client %d: ", i);
		display(i, wbuff);
		display(i, s);
		free(s);
		s = NULL;

	}


}

//int main() {
int main(int argc, char **argv) {

	if (argc != 2)
	{
		write(2, "Wrong number of arguments\n", 26);
		exit(1);
	}

	FD_ZERO(&fds);
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) { 
		ft_fatal(); 
	} 
	fdMax = sockfd;
	FD_SET(fdMax, &fds);

	struct sockaddr_in servaddr; 

	bzero(&servaddr, sizeof(servaddr)); 
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	servaddr.sin_port = htons(atoi(argv[1]));   //---------------------
  
	if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)))) { 
		ft_fatal();
	} 

	if (listen(sockfd, 10)) {
		ft_fatal();
	}

	while (1)
	{
		rfds = wfds = fds;

		if (select(fdMax + 1, &rfds, &wfds, NULL, NULL) < 0)
			ft_fatal();
		for (int i = 0; i <= fdMax; i++)
		{
			if (!FD_ISSET(i, &rfds))
				continue;
			
			if (i == sockfd)
			{
				socklen_t	len = sizeof(servaddr);
				int	client = accept(i, (struct sockaddr *)&servaddr, &len);
				if (client >= 0)
				{
					fdMax = client > fdMax ? client : fdMax;
					idx[client] = clients++;
					msg[client] = NULL;
					FD_SET(client, &fds);
					sprintf(wbuff, "server: client %d just arrived\n", idx[client]);
					display(client, wbuff);
					break;
				}
			}
			else 
			{
				int	rlen = recv(i, rbuff, 6000, 0);
				if (rlen <= 0)
				{
					sprintf(wbuff, "server: client %d just left\n", idx[i]);
					display(i, wbuff);
					free(msg[i]);
					msg[i] = NULL;

					FD_CLR(i, &fds);
					close(i);
					break;
				}
				rbuff[rlen] = '\0';
				msg[i] = str_join(msg[i], rbuff);
				delivre(i);
			}
		}
	}
}
