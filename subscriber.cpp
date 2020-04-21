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
#include <netinet/tcp.h>

#include "utils.h"

void usage(char *file)
{
	fprintf(stderr, "Usage: %s id_client server_address server_port\n", file);
	exit(0);
}

void wrong_command() {
	printf("\nComanda introdusa este invalida\n");
	printf("Comanda trebuie sa aiba una din structurile:\n");
	printf("subscribe topic_name 0/1\n");
	printf("unsubscribe topic_name\n\n");
}

/* returneaza -1 daca inputul este invalid, 0 daca s-a dat comanda exit si
1 daca totul este ok */
int check_correct_input_subscriber(const char *input) {
	char copy_input[BUFF_SIZE];
	memset(copy_input, 0, sizeof(copy_input));
	memcpy(copy_input, input, strlen(input));

	char *command = strtok(copy_input, " \n");
	if (strncmp(command, "exit", 4) == 0) {
		return 0;
	} else if (strncmp(command, "subscribe", 9) == 0) {
		char *topic = strtok(NULL, " \n");
		if (topic == NULL) {
			wrong_command();
			return -1;
		}

		char *sf = strtok(NULL, " \n");
		if (sf == NULL || (sf[0] != '1' && sf[0] != '0')) {
			wrong_command();
			return -1;
		}
	} else if (strncmp(command, "unsubscribe", 11) == 0) {
		char *topic = strtok(NULL, " \n");
		if (topic == NULL) {
			wrong_command();
			return -1;
		}
	} else {
		wrong_command();
		return -1;
	}

	return 1;
}

int main(int argc, char *argv[])
{
	if (argc != 4) {
		usage(argv[0]);
	}

	int fdmax = -1;
	int sockfd;
	int ret_code;
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd < 0, "socket");
	fdmax = std::max(fdmax, sockfd);

	// disable NIGLE Algorithm on this socket
	uint32_t disable = 1;
	ret_code = setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (char *) &disable, sizeof(uint32_t));
	DIE(ret_code < 0, "setsockopt");

	struct sockaddr_in serv_addr;

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[3]));
	ret_code = inet_aton(argv[2], &serv_addr.sin_addr);
	DIE(ret_code == 0, "inet_aton");

	ret_code = connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
	DIE(ret_code < 0, "connect");


	char buffer[BUFF_SIZE];
	memset(buffer, 0, sizeof(buffer));
	// format *msg = (format*) malloc(sizeof(format));
	// memset(msg, 0, sizeof(format));

	// trimite datele in formatul bun!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	format *msg_name = (format *) malloc(sizeof(format));
	memset(msg_name, 0, sizeof(format));

    memcpy(msg_name->content, argv[1], strlen(argv[1]));
	msg_name->len = 4 + strlen(argv[1]) + 1;
    ret_code = send(sockfd, msg_name, msg_name->len, 0);
	DIE(ret_code < 0, "send");

	fd_set read_fds;
	fd_set tmp_fds;

	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	// citire de la tastatura
	FD_SET(0, &read_fds);

	// socketul tcp
	FD_SET(sockfd, &read_fds);

	format *msg_to_send = (format *) malloc(sizeof(format));

	while (1) {
		tmp_fds = read_fds;
		
		ret_code = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret_code < 0, "select");

		if (FD_ISSET(0, &tmp_fds)) {
			memset(msg_to_send, 0, sizeof(format));
			fgets(msg_to_send->content, BUFF_SIZE, stdin);

			int ret_code = check_correct_input_subscriber(msg_to_send->content);
			if (ret_code == -1) {
				continue;
			} else if (ret_code == 0) {
				break;
			}

			msg_to_send->len = 4 + strlen(msg_to_send->content) + 1;
			ret_code = send(sockfd, msg_to_send, msg_to_send->len, 0);
			DIE(ret_code < 0, "send");

			char *sub_unsub = strtok(msg_to_send->content, " \n");
			char *topic = strtok(NULL, " \n");
			if (strncmp("subscribe", sub_unsub, 9) == 0) {
				printf("subscribed %s\n", topic);
			} else {
				printf("unsubscribed %s\n", topic);
			}
		} else if (FD_ISSET(sockfd, &tmp_fds)) {
			memset(buffer, 0, BUFF_SIZE);
			ret_code = recv(sockfd, buffer, BUFF_SIZE, 0);
			DIE(ret_code < 0, "recv");

			if (ret_code == 0) {
				printf("Serverul a inchis conexiunea\n");
				shutdown(sockfd, SHUT_RDWR);
				close(sockfd);
				return 0;
			}

			int recv_info_size = ret_code;
			int msg_recv_offset = 0;
			while (msg_recv_offset < recv_info_size) {
				format *msg_recv = (format*) (buffer + msg_recv_offset);
				printf("Am primit de la server: %s\n", msg_recv->content);
				msg_recv_offset += msg_recv->len;
			}
		}
	}

	shutdown(sockfd, SHUT_RDWR);
	close(sockfd);
	free(msg_to_send);
	return 0;
}
