//  test Wilhelm


#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>

int clients = 0, fd_max = 0;
int idx[65536];
char *msg[65536];
char rbuf[1025], wbuf[42];
fd_set fds, rfds, wfds;

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

void fatal()
{
	write(2, "Fatal error\n", 12);
	exit(1);
}

// envois un msg a tout les fd sauf au fd qui a envoye le message.
void notify(int from, char *s)
{
	for (int fd = 0; fd <= fd_max; fd++)
	{
		// Si le fd de la boucle est dans wfds(donc fds) et ce n'est pas le fd qui a envoye le msg.
		if (FD_ISSET(fd, &wfds) && fd != from)
			send(fd, s, strlen(s), 0); // on envois le msg au socket.
	}
}

void deliver(int fd)
{
	char *s;

	// permet de passer la string de msg[fd] a s
	while (extract_message(&(msg[fd]), &s))
	{
		// store client[%d] in wbuf.
		sprintf(wbuf, "client %d: ", idx[fd]);
		notify(fd, wbuf);
		notify(fd, s);
		free(s);
		s = NULL;
	}
}

void add_client(int client)
{
	fd_max = client > fd_max ? client : fd_max;
	// on ajoute le nb(+1 car idx start a 0) du client a idx(tableau index client pour "client %d:")
	idx[client] = clients++;
	msg[client] = NULL;
	// ajoute client a fds.
	FD_SET(client, &fds);
	sprintf(wbuf, "server: client %d just arrived\n", idx[client]);
	notify(client, wbuf);
}

void remove_client(int fd)
{
	sprintf(wbuf, "server: client %d just left\n", idx[fd]);
	notify(fd, wbuf);
	free(msg[fd]);
	msg[fd] = NULL;
	// remove le socket de fds.
	FD_CLR(fd, &fds);
	close(fd);
}

int create_socket(void)
{
	fd_max = socket(AF_INET, SOCK_STREAM, 0);	// creation socket.
	if (fd_max < 0)
		fatal();
	FD_SET(fd_max, &fds);	// ajout socket dans tableau fds.
	return (fd_max);
}

int main(int ac, char **av)
{
	if (ac != 2)
	{
		write(2, "Wrong number of arguments\n", 26);
		exit(1);
	}

	FD_ZERO(&fds);		// initialise fds a 0.

	int sockfd = create_socket();	// cree nouvelle socket : "socketfd".

	struct sockaddr_in servaddr;
	bzero(&servaddr, sizeof(servaddr));

	// assign IP, PORT
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	servaddr.sin_port = htons(atoi(av[1]));

	// associate socket with server
	if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)))
		fatal();
	// tell system that sockfd will be the socket for new connection.
	if (listen(sockfd, SOMAXCONN))
		fatal();
	while (42)
	{
		rfds = wfds = fds;

		// Ckeck si un socket est pret a l'ecriture / lecture, puis ajoute le socket en question ds rfds.
		// Si nvl connection met "sockfd" dans rfds. 
		if (select(fd_max + 1, &rfds, &wfds, NULL, NULL) < 0)
			fatal();
		for (int fd = 0; fd <= fd_max; fd++)
		{
			// si fd pas present dans rfds, alors on loop.
			if (!FD_ISSET(fd, &rfds))
				continue ;

			// Si le fd dans rfds == nouveau socket
			if (fd == sockfd)
			{
				socklen_t addr_len = sizeof(servaddr);
				// accept connexion entre nv socket "client" et le server.
				int client = accept(sockfd, (struct sockaddr *)&servaddr, &addr_len);
				if (client >= 0)
				{
					// ajoute socket "client" ds fds.
					add_client(client);
					break ;
				}
			}

			// Si le fd dans rfds pas nouveau socket
			else
			{
				// fonction pour recevoir msg.
				// rbug = buffer vide, 1024 max char for buf.
				// retourne longeur du buff.
				int readed = recv(fd, rbuf, 1024, 0);
				if (readed <= 0)
				{
					remove_client(fd);
					break ;
				}
				rbuf[readed] = '\0';
				// add buff(receiv msg) into msg[fd]
				msg[fd] = str_join(msg[fd], rbuf);
				// envois msg aux autres clients.
				deliver(fd);
			}
		}
	}
	return (0);
}

