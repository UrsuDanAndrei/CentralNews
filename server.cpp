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

void add_tcp_client(int& fdmax, int sockfd_tcp_listen, fd_set &read_fds,
													std::vector<int> &to_add) {
	int ret_code;
	char buffer[BUFF_SIZE];
    memset(buffer, 0, sizeof(buffer));

	printf("in add_tcp_client\n");
	// a venit o cerere de conexiune pe socketul inactiv (cel cu listen),
	// pe care serverul o accepta
	struct sockaddr_in client_addr;
	socklen_t client_len = sizeof(client_addr);
	int new_sockfd = accept(sockfd_tcp_listen, (struct sockaddr *) &client_addr, &client_len);
	DIE(new_sockfd < 0, "accept");

	// se adauga noul socket intors de accept() la multimea descriptorilor de citire
	FD_SET(new_sockfd, &read_fds);
	fdmax = std::max(fdmax, new_sockfd);
    to_add.push_back(new_sockfd);

    // se primeste primul mesaj cu numele de autentificare (client_id)
	ret_code = recv(new_sockfd, buffer, sizeof(buffer), 0);
	DIE(ret_code < 0, "recv");
    //int msg_len = ret_code;

	printf("New client %s connected from %s:%d.\n",
		buffer ,inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
}

void process_tcp_client_request(int sockfd, fd_set& read_fds,
												std::vector<int>& to_delete) {
	int ret_code;
    char buffer[BUFF_SIZE];
	memset(buffer, 0, sizeof(buffer));

    printf("in process_tcp_client_request\n");
	// s-au primit date pe unul din socketii de client,
	// asa ca serverul trebuie sa le receptioneze
	ret_code = recv(sockfd, buffer, sizeof(buffer), 0);
	DIE(ret_code < 0, "recv");

	if (ret_code == 0) {
		// conexiunea s-a inchis
		printf("Client (CLIENT_ID) disconnected. %d\n", sockfd);

		// se scoate din multimea de citire socketul inchis 
		// se elimina clientul din read_fds si se va sterge din lista de socketi
		// activi
		FD_CLR(sockfd, &read_fds);
		to_delete.push_back(sockfd);

		// closing connection
		shutdown(sockfd, SHUT_RDWR);
		close(sockfd);
	} else {
		printf ("S-a primit de la clientul de pe socketul %d mesajul: %s\n", sockfd, buffer);
	}
}

/* returneaza 1 daca executia poate continua dupa comanda primita de la,
tastatura, 0 altfel */
int command_from_stdin(std::unordered_set<int> &all_sockets) {
	char buffer[COMMAND_LEN];
	memset(buffer, 0, sizeof(buffer));

	fgets(buffer, sizeof(buffer), stdin);

	if (strncmp(buffer, "exit", 4) == 0) {
		// inchide toate conexiunile inainte de a termina executia programuluo
		for (auto it = all_sockets.begin(); it != all_sockets.end(); ++it) {
			int fd = *it;
			shutdown(fd, SHUT_RDWR);
			close(fd);		
		}

		return 0;
	} else {
		// exit este singura comanda specificata in enunt
		printf("Comanda introdusa este invalida!\n");
		printf("Lista de comenzi valide:\n");
		printf("exit\n\n");

		return 1;
	}
}

// !!! intreaba de problema cu exit, dc nu se inchide socketul si se inchide cand dau ctrl^C

void process_received_info(int sockfd) {
	printf("din process_received_info\n");

	char buffer[BUFF_SIZE];
	memset(buffer, 0, sizeof(buffer));

	struct sockaddr_in client_addr;
	socklen_t client_len = sizeof(client_addr);


	// !!! MSG_WAITALL poate trebuie altceva
	recvfrom(sockfd, buffer, sizeof(buffer), MSG_WAITALL,
								(struct sockaddr*) &client_addr, &client_len);
	
	// the buffer will be parsed into msg
	char msg[BUFF_SIZE];
	memset(msg, 0, sizeof(msg));
	
	// !!! poate are 50 bytes fix si n-are termnator de sir
	// !!! inet_ntoa are cod de eroare

	// IP
	char *ip_addr = inet_ntoa(client_addr.sin_addr);
	memcpy(msg, ip_addr, strlen(ip_addr));

	// IP:
	int pct2_offset = strlen(ip_addr);
	msg[pct2_offset] = ':';

	// IP:PORT
	int port_offset = pct2_offset + 1;
	int port_no_len = sprintf(msg + port_offset, "%d", ntohs(client_addr.sin_port));

	// IP:PORT - 
	int s_line_s_offset = port_offset + port_no_len;
	msg[s_line_s_offset] = ' ';
	msg[s_line_s_offset + 1] = '-';
	msg[s_line_s_offset + 2] = ' ';

	/* strlen returneaza dimensiunea pana la primul '\0' din buffer, adica exact
	lungimea topicului */
	// IP:PORT - topic
	int topic_offset = s_line_s_offset + 3;
	memcpy(msg + topic_offset, buffer, strlen(buffer));
	
	// IP:PORT - topic - 
	int s_line_s_offset2 = topic_offset + strlen(buffer);
	msg[s_line_s_offset2] = ' ';
	msg[s_line_s_offset2 + 1] = '-';
	msg[s_line_s_offset2 + 2] = ' ';

	// IP:PORT - topic - tip_date - valoare mesaj
	int type_offset =  s_line_s_offset2 + 3;
	uint8_t type;
	memcpy(&type, buffer + TYPE_OFFSET, 1);

	// !!! poate denumeste toate tipurele intr-un enum or something
	switch (type) {
		case INT: {
			msg[type_offset] = 'I';
			msg[type_offset + 1] = 'N';
			msg[type_offset + 2] = 'T';

			int s_line_s_offset3 = type_offset + 3;
			msg[s_line_s_offset3] = ' ';
			msg[s_line_s_offset3 + 1] = '-';
			msg[s_line_s_offset3 + 2] = ' ';

			uint8_t sign;
			memcpy(&sign, buffer + SIGN_OFFSET, 1);

			uint32_t no;
			memcpy(&no, buffer + INT_OFFSET, sizeof(uint32_t));
			no = ntohl(no);

			int no_offset = s_line_s_offset3 + 3;
			if (sign == 1) {
				msg[no_offset] = '-';
				++no_offset;
			}

			memcpy(msg + no_offset, &no, sizeof(uint32_t));
			int no_len = sprintf(msg + no_offset, "%d", no);

			int final_len = no_offset + no_len;
			msg[final_len] = '\0';

			break;
		}
		
		case SHORT_REAL: {
			msg[type_offset] = 'S';
			msg[type_offset + 1] = 'H';
			msg[type_offset + 2] = 'O';
			msg[type_offset + 3] = 'R';
			msg[type_offset + 4] = 'T';
			msg[type_offset + 5] = '_';
			msg[type_offset + 6] = 'R';
			msg[type_offset + 7] = 'E';
			msg[type_offset + 8] = 'A';
			msg[type_offset + 9] = 'L';

			int s_line_s_offset3 = type_offset + 10;
			msg[s_line_s_offset3] = ' ';
			msg[s_line_s_offset3 + 1] = '-';
			msg[s_line_s_offset3 + 2] = ' ';

			uint16_t no;
			memcpy(&no, buffer + SHORT_REAL_OFFSET, sizeof(uint16_t));
			no = ntohs(no);

			int no_offset = s_line_s_offset3 + 3;
			int no_len = sprintf(msg + no_offset, "%d", no);

			// se muta ultimele 2 cifre cu o pozitie la dreapta
			// for (int i = no_offset + no_len; i >= no_offset + no_len - 1; --i) {
			// 	msg[i] = msg[i - 1];
			// }
			memcpy(msg + no_offset + no_len - 1, msg + no_offset + no_len - 2, 2);


			// !!! poate nu vrea sa completezi cu 0, gen 17 -> 17 nu 17 -> 17.00, ca
			// la float de exemplu 
			// se plaseaza caracterul '.'
			int dot_offset = no_offset + no_len - 2;
			msg[dot_offset] = '.';

			int final_len = no_offset + no_len + 1;
			msg[final_len] = '\0';

			break;
		}

		case FLOAT: {
			msg[type_offset] = 'F';
			msg[type_offset + 1] = 'L';
			msg[type_offset + 2] = 'O';
			msg[type_offset + 3] = 'A';
			msg[type_offset + 4] = 'T';

			int s_line_s_offset3 = type_offset + 5;
			msg[s_line_s_offset3] = ' ';
			msg[s_line_s_offset3 + 1] = '-';
			msg[s_line_s_offset3 + 2] = ' ';

			// se plaseaza semnul '-' daca este cazul
			uint8_t sign;
			memcpy(&sign, buffer + SIGN_OFFSET, sizeof(uint8_t));

			int no_offset = s_line_s_offset3 + 3;
			if (sign == 1) {
				msg[no_offset] = '-';
				++no_offset;
			}

			uint32_t no;
			memcpy(&no, buffer + FLOAT_OFFSET, sizeof(uint32_t));
			no = ntohl(no);
			int no_len = sprintf(msg + no_offset, "%d", no);

			uint8_t power10;
			memcpy(&power10, buffer + POWER10_OFFSET, sizeof(uint8_t));

			/* numarul de zerouri ce trebuie adaugate pentru o
			reprezentare corecta a numarului */
			int no_zeros = power10 - no_len + 1;
			if (no_zeros > 0) {
				/* se deplaseaza numarul cu nr_zeros la dreapta pentru a face loc 
				pentru zero-urile ce urmeaza a fi adaugate */
				memcpy(msg + no_offset + no_zeros, msg + no_offset, no_len);
				no_len += no_zeros;
				
				// se adauga zero-urile
				while (no_zeros != 0) {
					msg[no_offset + no_zeros - 1] = '0';
					--no_zeros;
				}
			}
			
			if (power10 != 0) {
				// se plaseaza caracterul '.'
				memcpy(msg + no_offset + no_len - (power10 - 1), msg + no_offset + no_len - power10, power10);

				int dot_offset = no_offset + no_len - power10;
				msg[dot_offset] = '.';

				int final_len = no_offset + no_len + 1;
				msg[final_len] = '\0';
			}

			break;
		}

		case STRING: {
			msg[type_offset] = 'S';
			msg[type_offset + 1] = 'T';
			msg[type_offset + 2] = 'R';
			msg[type_offset + 3] = 'I';
			msg[type_offset + 4] = 'N';
			msg[type_offset + 5] = 'G';

			int s_line_s_offset3 = type_offset + 6;
			msg[s_line_s_offset3] = ' ';
			msg[s_line_s_offset3 + 1] = '-';
			msg[s_line_s_offset3 + 2] = ' ';

			// !!! atentie la strcyp 
			int string_offset = s_line_s_offset3 + 3;
			strncpy(msg + string_offset, buffer + STRING_OFFSET, MAX_STRING_SIZE);

			break;
		}
	}

	// !!! poate nu se temina cu '\0' daca are fix 1500 caractere, strcpy nasol atunci
	/// !!! poate mesajele de la UDP client nu au formatul specificat, trebuie testa eventual

	printf("UDP Clinet spune: %s\n", msg);

}

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
					process_received_info(fd);
                } else if (fd == sockfd_tcp_listen) {
					add_tcp_client(fdmax, sockfd_tcp_listen, read_fds, to_add);
				} else {
					process_tcp_client_request(fd, read_fds, to_delete);
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
