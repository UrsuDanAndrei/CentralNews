#include "client.h"
#include "tcp_client_handler.h"
#include "udp_client_handler.h"
#include "utils.h"

#include <arpa/inet.h>
#include <bits/stdc++.h>
#include <iostream>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>

/* returneaza 1 daca executia poate continua dupa comanda primita de la
tastatura, 0 altfel */
int command_from_stdin(std::unordered_set<int> &all_sockets) {
	char buffer[COMMAND_LEN];
	memset(buffer, 0, sizeof(buffer));

	fgets(buffer, sizeof(buffer), stdin);

	if (strncmp(buffer, "exit", 4) == 0) {
		// inchide toate conexiunile inainte de a termina executia programului
		for (auto it = all_sockets.begin(); it != all_sockets.end(); ++it) {
			int fd = *it;
			shutdown(fd, SHUT_RDWR);
			close(fd);		
		}

		return 0;
	} else {
		// exit este singura comanda specificata in enunt
		printf("\nComanda introdusa este invalida!\n");
		printf("Lista de comenzi valide:\n");
		printf("exit\n\n");

		return 1;
	}
}

// !!! intreaba de problema cu exit, dc nu se inchide portul cand dau exit si se inchide cand dau ctrl^C
// !!! sa se poata adauga topics si in tcp subscribe on topic
// !!! poate denumeste toate tipurele intr-un enum or something


// !!! m-am gandit sa atribui fiecarui client

void usage(char *file)
{
	fprintf(stderr, "Usage: %s server_port\n", file);
	exit(0);
}

// !!! exit-ul nu merge dupa ce inchid clientii!!!! poate shutdown e o problema
// !!! poate nu inchid toti clientii

int main(int argc, char *argv[])
{
	if (argc < 2) {
		usage(argv[0]);
	}

	int ret_code;

	struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));

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

	char buffer[BUFF_SIZE];
	memset(buffer, 0, sizeof(buffer));

	// !!! unorderde map pentru clineti pt a permite o usoaraa ditribuire de socketi,
	// si eficienta in memorie
	int max_cli_id = 0;
	std::unordered_map<std::string, int> cli2id;
	std::unordered_map<std::string, std::unordered_set<int>> topic_subs;
	std::unordered_map<int, int> sockfd2cli;
	std::vector<Client> clis;

	while (1) {
		tmp_fds = read_fds;
		
		ret_code = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret_code < 0, "select");

		std::vector<int> to_add;
		std::vector<int> to_delete;

		for (auto it = all_sockets.begin(); it != all_sockets.end(); ++it) {
			int fd = *it;
			if (FD_ISSET(fd, &tmp_fds)) {
				if (fd == 0) {
					// citirea de la tastatura
					ret_code = command_from_stdin(all_sockets);
					if (ret_code == 0) {
						return 0;
					}
				} else if (fd == sockfd_udp) {
                    // adaug mesajele primite
					process_received_info(fd, clis, topic_subs);
                } else if (fd == sockfd_tcp_listen) {
					add_tcp_client(fdmax, sockfd_tcp_listen, read_fds, to_add,
										sockfd2cli, cli2id, clis, max_cli_id);
				} else {
					// s-au primit date pe unul din socketii de client,
					// asa ca serverul trebuie sa le receptioneze
					std::vector<std::string> msgs;
					ret_code = get_parsed_messages(fd, msgs);

					int id = sockfd2cli[fd];
					Client& cli = clis[id];

					std::cout << "Client name is: " << cli.name << " and id is: " << id << std::endl;

					if (ret_code == 0) {
						// conexiunea s-a inchis
						//printf("Client gigel disconnected. %d\n", sockfd);
						std::cout << "Client " << cli.name
											<< " disconected" << std::endl;

						// se scoate din multimea de citire socketul inchis 
						// se elimina clientul din read_fds si se va sterge din lista de socketi
						// activi
						FD_CLR(fd, &read_fds);
						to_delete.push_back(fd);

						cli.on = false;
						sockfd2cli.erase(fd);

						// closing connection
						shutdown(fd, SHUT_RDWR);
						close(fd);

						continue;
					}

					for (std::string& str : msgs) {
						char* char_str = (char*) malloc(str.size() + 1);
						memcpy(char_str, str.c_str(), str.size() + 1);
						char_str[str.size()] = '\0';

						process_tcp_client_request(fd, read_fds, char_str, to_delete,
												sockfd2cli, clis, topic_subs);
						free(char_str);
					}
				}
			}
		}

		// deletes the closed connections
		for (int sock : to_delete) {
			all_sockets.erase(sock);
		}

		// adds all new connections
		for (int sock : to_add) {
			all_sockets.insert(sock);
		}
	}

	return 0;
}
