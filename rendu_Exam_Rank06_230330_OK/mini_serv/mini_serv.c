/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   mini_serv.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: pwolff <pwolff@student.42mulhouse.fr>>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/03/29 08:46:05 by pwolff            #+#    #+#             */
/*   Updated: 2023/03/30 18:29:11 by pwolff           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/* **************  ExamRank06 du 30/03/2023  OK  ****************** */

#include <errno.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>

fd_set	rfd, wfd, fds;
char	rbuf[65536], wbuf[65536];
char	*msg[65536];
int 	fdMax = 0, clients = 0;
int  	idx[65536];

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

void	fatal(void)
{
	write(2, "Fatal error\n", 12);
	exit(1);
}

void	display(int client, char *wbuf)
{
	for (int i = 0; i <= fdMax; i++)
	{
		if (FD_ISSET(i, &wfd) && i != client)
			send(i, wbuf, strlen(wbuf), 0);
	}

}

int main(int argc, char **argv) {
	if (argc != 2)
	{
		write(2, "Wrong number of arguments\n", 26);
		exit(1);
	}
	struct sockaddr_in servaddr; 

	int sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (sockfd < 0) { 
		fatal();
	} 

	FD_ZERO(&fds);
	fdMax = sockfd;
	FD_SET(fdMax, &fds);
	bzero(&servaddr, sizeof(servaddr)); 

	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	servaddr.sin_port = htons(atoi(argv[1])); 
  
	// Binding newly created socket to given IP and verification 
	if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)))) { 
		fatal();
	} 
	if (listen(sockfd, 10)) {
		fatal();
	}
	while(42)
	{
		rfd = wfd = fds;
		if (select (fdMax + 1, &rfd, &wfd, NULL, NULL) <0 )
			fatal();
		for (int i = 0; i <= fdMax; i++)
		{
			if (!FD_ISSET(i, &rfd))
				continue;
			if (i == sockfd)
			{
				socklen_t len = sizeof(servaddr);
				int client = accept(sockfd, (struct sockaddr *)&servaddr, &len);
				if (client >= 0)
				{
					fdMax = client > fdMax ? client : fdMax;
					idx[client] = clients++;
					msg[client] = NULL;
					FD_SET(client, &fds);
					sprintf(wbuf, "server: client %d just arrived\n", idx[client]);
					display(client, wbuf);
					break;
				}
			}
			else
			{
				int rlen = recv(i, rbuf, 6000, 0);
				if (rlen <= 0)
				{
					sprintf(wbuf, "server: client %d just left\n", idx[i]);
					display(i, wbuf);
					FD_CLR(i, &fds);
					free(msg[i]);
					msg[i] = NULL;
					close (i);
					break;
				}
				char	*s;
				rbuf[rlen] = '\0';
				msg[i] = str_join(msg[i], rbuf);
				while (extract_message(&(msg[i]), &s))
				{
					sprintf(wbuf, "client %d: ", idx[i]);
					display(i, wbuf);
					display(i, s);
					free(s);
					s =NULL;

				}
			}
		}
	}
}
