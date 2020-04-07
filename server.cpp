#include <bits/stdc++.h>
#include <vector>

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
	int sockfd, newsockfd, ret_code;
	char buffer[BUFLEN];
	int i;
	//socklen_t clilen;

	fd_set read_fds;	// multimea de citire folosita in select()
	fd_set tmp_fds;		// multime folosita temporar
	int fdmax = -1;			// valoare maxima fd din multimea read_fds

	if (argc < 2) {
		usage(argv[0]);
	}

	// se goleste multimea de descriptori de citire (read_fds) si multimea temporara (tmp_fds)
	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

    //---------------
    std::vector<int> all_sockets;

	struct sockaddr_in serv_addr, cli_addr;
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

	ret_code = listen(sockfd_tcp_listen, MAX_CLIENTS);
	DIE(ret_code < 0, "listen");

	// se adauga socketfd_tcp
	FD_SET(sockfd_tcp_listen, &read_fds);
	fdmax = std::max(fdmax, sockfd_tcp_listen);
    all_sockets.push_back(sockfd_tcp_listen);

    // se adauga socketfd_udp
    FD_SET(sockfd_udp, &read_fds);
	fdmax = std::max(fdmax, sockfd_udp);
    all_sockets.push_back(sockfd_udp);



	// -----------------------

	// clilen = sizeof(cli_addr);
	// int sockfd1 = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
	// DIE(sockfd1 < 0, "accept");

	// printf("Am primit primul client\n");

	// int sockfd2 = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
	// DIE(sockfd2 < 0, "accept");

	// printf("Am primit al doilea client\n");

	// FD_SET(sockfd1, &read_fds);
	// FD_SET(sockfd2, &read_fds);

	// if (fdmax < sockfd1) {
	// 	fdmax = sockfd1;
	// }

	// if (fdmax < sockfd2) {
	// 	fdmax = sockfd2;
	// }
	//int sockfd1=0, sockfd2=0;
	// ------------------------

	while (1) {
		tmp_fds = read_fds;
		
		ret_code = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret_code < 0, "select");

        int socket_no = all_sockets.size();
		for (i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &tmp_fds)) {
				if (i == sockfd_tcp_listen) {
					// a venit o cerere de conexiune pe socketul inactiv (cel cu listen),
					// pe care serverul o accepta
					socklen_t client_len = sizeof(cli_addr);
					newsockfd = accept(sockfd_tcp_listen, (struct sockaddr *) &cli_addr, &client_len);
					DIE(newsockfd < 0, "accept");

					// se adauga noul socket intors de accept() la multimea descriptorilor de citire
					FD_SET(newsockfd, &read_fds);
					fdmax = std::max(fdmax, newsockfd);
                    all_sockets.push_back(fdmax);

                    // se primeste primul mesaj cu numele de autentificare (client_id)
                    memset(buffer, 0, BUFLEN);
					ret_code = recv(newsockfd, buffer, sizeof(buffer), 0);
					DIE(ret_code < 0, "recv");
                    int msg_len = ret_code;

					printf("New client %s connected from %s:%d.\n",
							buffer ,inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
				} else if (i == sockfd_udp) {
                    // adaug mesajele primite
                } else {
					// s-au primit date pe unul din socketii de client,
					// asa ca serverul trebuie sa le receptioneze
					memset(buffer, 0, BUFLEN);
					ret_code = recv(i, buffer, sizeof(buffer), 0);
					DIE(ret_code < 0, "recv");

					if (ret_code == 0) {
						// conexiunea s-a inchis
						printf("Client (CLIENT_ID) disconnected. %d\n", i);
						close(i);
						
						// se scoate din multimea de citire socketul inchis 
						FD_CLR(i, &read_fds);
						continue;
					} else {
						printf ("S-a primit de la clientul de pe socketul %d mesajul: %s\n", i, buffer);
					}
				}
			}
		}
	}

	close(sockfd);

	return 0;
}
