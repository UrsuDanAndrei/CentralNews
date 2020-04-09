#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "utils.h"

void usage(char *file)
{
	fprintf(stderr, "Usage: %s server_address server_port\n", file);
	exit(0);
}

int main(int argc, char *argv[])
{
	//setvbuf(stdout, NULL, _IONBF, 0);
	int fdmax;			// valoare maxima fd din multimea read_fds
	int sockfd, n, ret;
	struct sockaddr_in serv_addr;
	char buffer[BUFLEN];

	if (argc < 3) {
		usage(argv[0]);
	}

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd < 0, "socket");

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[2]));
	ret = inet_aton(argv[1], &serv_addr.sin_addr);
	DIE(ret == 0, "inet_aton");

	ret = connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr));

    // isi trimite baitul numele
    memcpy(buffer, "gigel\0", 6);
    n = send(sockfd, buffer, strlen(buffer), 0);
	DIE(n < 0, "send");

	DIE(ret < 0, "connect");

	fdmax = sockfd;

	fd_set read_fds;	// multimea de citire folosita in select()
	fd_set write_fds;

	fd_set tmp_read_fds;		// multime folosita temporar
	fd_set tmp_write_fds;

	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_read_fds);

	FD_ZERO(&write_fds);
	FD_ZERO(&tmp_write_fds);

	// citire de la tastatura
	FD_SET(0, &read_fds);

	FD_SET(sockfd, &read_fds);
	// FD_SET(sockfd, &write_fds);

	struct sockaddr_in helper;

	while (1) {
		// FD_ZERO(&tmp_write_fds);
		FD_ZERO(&tmp_read_fds);
		tmp_read_fds = read_fds; 
		// tmp_write_fds = write_fds;
		
		ret = select(fdmax + 1, &tmp_read_fds, NULL, NULL, NULL);
		DIE(ret < 0, "select");

		if (FD_ISSET(sockfd, &tmp_read_fds)){
			//printf("ieeeeeee22222222222\n");
			memset(buffer, 0, BUFLEN);

			n = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
			DIE(n < 0, "recv");

			if (n == 0) {
				printf("Serverul a inchis conexiunea\n");
				break;
			}

			printf("Am primit de la server: %s\n", buffer);
		} else if (FD_ISSET(0, &tmp_read_fds)){
		//	printf("ieeeeeee\n");
  			// se citeste de la tastatura
			memset(buffer, 0, BUFLEN);
			fgets(buffer, BUFLEN - 1, stdin);

			if (strncmp(buffer, "exit", 4) == 0) {
				break;
			}

			// se trimite mesaj la server
			n = send(sockfd, buffer, strlen(buffer), 0);
			DIE(n < 0, "send");
		}
	}

	shutdown(sockfd, 2);
	close(sockfd);

	return 0;
}
