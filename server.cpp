#include <bits/stdc++.h>
#include <vector>
#include <set>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "utils.h"

void usage(char *file)
{
	fprintf(stderr, "Usage: %s server_port\n", file);
	exit(0);
}

int main(int argc, char *argv[])
{
	if (argc < 2) {
		usage(argv[0]);
	}

	int ret_code;

	struct sockaddr_in serv_addr;
    memset((char *) &serv_addr, 0, sizeof(serv_addr));

	int port_no = atoi(argv[1]);
	DIE(port_no == 0, "atoi");

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port_no);
	serv_addr.sin_addr.s_addr = INADDR_ANY;

    // TCP socket
	int sockfd_tcp_listen = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd_tcp_listen < 0, "socket");
	ret_code = bind(sockfd_tcp_listen, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr));
	DIE(ret_code < 0, "bind");

    // UDP socket
    int sockfd_udp = socket(AF_INET, SOCK_DGRAM, 0);
	DIE(sockfd_udp < 0, "socket");
    ret_code = bind(sockfd_udp, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr));
	DIE(ret_code < 0, "bind");

	// start listening for clients
	ret_code = listen(sockfd_tcp_listen, MAX_CLIENTS);
	DIE(ret_code < 0, "listen");

	std::unordered_set<int> all_sockets;
	int fdmax = -1;

	fd_set read_fds;
	fd_set tmp_fds;

	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	// se adauga citirea de la tastatura
	FD_SET(0, &read_fds);
	fdmax = std::max(fdmax, 0);
	all_sockets.insert(0);

    // se adauga socketfd_udp
    FD_SET(sockfd_udp, &read_fds);
	fdmax = std::max(fdmax, sockfd_udp);
    all_sockets.insert(sockfd_udp);

	// se adauga socketfd_tcp
	FD_SET(sockfd_tcp_listen, &read_fds);
	fdmax = std::max(fdmax, sockfd_tcp_listen);
    all_sockets.insert(sockfd_tcp_listen);

	char buffer[BUFLEN];
	memset(buffer, 0, sizeof(buffer));

	while (1) {
		tmp_fds = read_fds;
		
		ret_code = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret_code < 0, "select");

        int socket_no = all_sockets.size();
		printf("size: %d\n", all_sockets.size());

		std::vector<int> to_add;
		std::vector<int> to_delete;

		for (auto it = all_sockets.begin(); it != all_sockets.end(); ++it) {
			printf("in interior\n");
			int i = *it;
			if (FD_ISSET(i, &tmp_fds)) {
				if (i == sockfd_tcp_listen) {
					printf("in listen\n");
					// a venit o cerere de conexiune pe socketul inactiv (cel cu listen),
					// pe care serverul o accepta
					struct sockaddr_in cli_addr;
					socklen_t client_len = sizeof(cli_addr);
					int new_sockfd = accept(sockfd_tcp_listen, (struct sockaddr *) &cli_addr, &client_len);
					DIE(new_sockfd < 0, "accept");

					// se adauga noul socket intors de accept() la multimea descriptorilor de citire
					FD_SET(new_sockfd, &read_fds);
					fdmax = std::max(fdmax, new_sockfd);
                    to_add.push_back(new_sockfd);

                    // se primeste primul mesaj cu numele de autentificare (client_id)
                    memset(buffer, 0, BUFLEN);
					ret_code = recv(new_sockfd, buffer, sizeof(buffer), 0);
					DIE(ret_code < 0, "recv");
                    int msg_len = ret_code;

					printf("New client %s connected from %s:%d.\n",
							buffer ,inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
				} else if (i == sockfd_udp) {
                    // adaug mesajele primite
                } else {
					printf("in else\n");
					// s-au primit date pe unul din socketii de client,
					// asa ca serverul trebuie sa le receptioneze
					memset(buffer, 0, BUFLEN);
					ret_code = recv(i, buffer, sizeof(buffer), 0);
					DIE(ret_code < 0, "recv");

					if (ret_code == 0) {
						// conexiunea s-a inchis
						printf("Client (CLIENT_ID) disconnected. %d\n", i);
						// se scoate din multimea de citire socketul inchis 
						FD_CLR(i, &read_fds);

						to_delete.push_back(i);
						close(i);
						
						continue;
					} else {
						printf ("S-a primit de la clientul de pe socketul %d mesajul: %s\n", i, buffer);
					}
				}
			}
		}

		// deletes the close connection
		for (int sock : to_delete) {
			all_sockets.erase(sock);
		}

		// adds all new connections
		for (int sock : to_add) {
			all_sockets.insert(sock);
		}
	}

	close(sockfd_tcp_listen);

	return 0;
}
