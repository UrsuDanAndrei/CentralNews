#include <bits/stdc++.h>
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
	fprintf(stderr, "Usage: %s server_address server_port nume_client\n", file);
	exit(0);
}

int main(int argc, char *argv[])
{
	if (argc != 4) {
		usage(argv[0]);
	}

	int fdmax = -1;
	int sockfd;
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd < 0, "socket");
	fdmax = std::max(fdmax, sockfd);

	int ret_code;
	struct sockaddr_in serv_addr;

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[2]));
	ret_code = inet_aton(argv[1], &serv_addr.sin_addr);
	DIE(ret_code == 0, "inet_aton");

	ret_code = connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
	DIE(ret_code < 0, "connect");

	char buffer[BUFF_SIZE];
	memset(buffer, 0, sizeof(buffer));

    memcpy(buffer, argv[3], strlen(argv[3]));
    ret_code = send(sockfd, buffer, strlen(buffer), 0);
	DIE(ret_code < 0, "send");

	fd_set read_fds;
	fd_set tmp_fds;

	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	// citire de la tastatura
	FD_SET(0, &read_fds);

	// socketul tcp
	FD_SET(sockfd, &read_fds);

	while (1) {
		tmp_fds = read_fds;
		
		ret_code = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret_code < 0, "select");

		if (FD_ISSET(0, &tmp_fds)){
			memset(buffer, 0, BUFF_SIZE);
			fgets(buffer, sizeof(buffer) - 1, stdin);

			if (strncmp(buffer, "exit", 4) == 0) {
				break;
			}

			ret_code = send(sockfd, buffer, strlen(buffer), 0);
			DIE(ret_code < 0, "send");
		} else if (FD_ISSET(sockfd, &tmp_fds)){
			memset(buffer, 0, BUFF_SIZE);
			ret_code = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
			DIE(ret_code < 0, "recv");

			if (ret_code == 0) {
				printf("Serverul a inchis conexiunea\n");
				shutdown(sockfd, SHUT_RDWR);
				close(sockfd);
				return 0;
			}

			printf("Am primit de la server: %s\n", buffer);
		}
	}

	return 0;
}
