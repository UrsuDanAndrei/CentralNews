#include "server.h"

/* returneaza 1 daca executia poate continua dupa comanda primita de la
tastatura, 0 altfel */
int command_from_stdin(std::unordered_set<int> &all_sockets) {
	char buffer[COMMAND_LEN];
	memset(buffer, 0, COMMAND_LEN);

	char* ret_ptr = fgets(buffer, sizeof(buffer), stdin);
	DIE(ret_ptr == NULL, "fgets");

	if (strncmp(buffer, "exit", 4) == 0) {
		// inchide toate conexiunile inainte de a termina executia programului
		for (auto it = all_sockets.begin(); it != all_sockets.end(); ++it) {
			int fd = *it;
			shutdown(fd, SHUT_RDWR);
			close(fd);	
		}

		return 0;
	} else {
		// exit este singura comanda valida specificata in enunt
		// std::cout << "\nComanda introdusa este invalida!\n";
		// std::cout << "Lista de comenzi valide:\n";
		// std::cout << "exit\n\n";

		return 1;
	}
}

void usage(char *file)
{
	// fprintf(stderr, "Utilizare: %s server_port\n", file);
	exit(0);
}

int main(int argc, char *argv[])
{
	// verifica daca apelul este realizat cu numarul corect de argumente
	if (argc < 2) {
		usage(argv[0]);
	}

	// variabila utilizata pentru verificarea eventualelor coduri de eroare
	int ret_code;

	// se initializeaza serv_addr cu adresa ip si portul serverului
	struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));

	int port_no = atoi(argv[1]);
	DIE(port_no == 0, "atoi");

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port_no);
	serv_addr.sin_addr.s_addr = INADDR_ANY; // inet_addr("alta adresa")

    /* se creaza socket-ul TCP pe care se va asculta pentru eventuale conexiuni
	si se leaga la portul stabilit anterior */
	int sockfd_tcp_listen = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd_tcp_listen < 0, "socket");

	int reuse = 1;
	ret_code = setsockopt(sockfd_tcp_listen, SOL_SOCKET, SO_REUSEADDR,
														&reuse, sizeof(int));
	DIE(ret_code < 0, "setsockopt");
	

	ret_code = bind(sockfd_tcp_listen, (struct sockaddr *) &serv_addr,
													sizeof(struct sockaddr));
	DIE(ret_code < 0, "bind");

    /* se creaza socket-ul UDP si se leaga la portul stabilit anterior */
    int sockfd_udp = socket(AF_INET, SOCK_DGRAM, 0);
	DIE(sockfd_udp < 0, "socket");
    ret_code = bind(sockfd_udp, (struct sockaddr *) &serv_addr,
													sizeof(struct sockaddr));
	DIE(ret_code < 0, "bind");

	// se pregateste server-ul pentru primirea conexiunilor TCP
	ret_code = listen(sockfd_tcp_listen, MAX_CLIENTS);
	DIE(ret_code < 0, "listen");

	// in acest set se vor retine toate socket-urile deschise la un moment dat
	std::unordered_set<int> all_sockets;
	int fdmax = -1;

	/* in read_fds se vor retine toti file descriptori pentru care este de
	asteptat sa primeasca informatii */
	fd_set read_fds;
	fd_set tmp_fds;

	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	// citirea de la stdin
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

	// buffer utilizat pentru diverse citiri
	char buffer[BUFF_SIZE];
	memset(buffer, 0, BUFF_SIZE);

	int max_cli_id = 0;

	// map de la numele clientului la id-ul acestuia
	std::unordered_map<std::string, int> cli2id;

	// pentru fiecare topic se retine o lista de subscriberi sub forma unui set
	std::unordered_map<std::string, std::unordered_set<int>> topic_subs;

	// map de la file descriptor la id-ul clientului care ii corespunde
	std::unordered_map<int, int> sockfd2cli;

	// vectorul de clienti
	std::vector<Client> clis;

	while (1) {
		tmp_fds = read_fds;
		
		// se asteapta o operatie de citire
		ret_code = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret_code < 0, "select");

		/* pentru adaugari si stergeri "safe" (nu in timpul parcurgerii) in
		set-ul all_sockets */
		std::vector<int> to_add;
		std::vector<int> to_delete;

		/* pentru fiecare socket din set se verifica daca se pot face operatii
		asupra acestuia */
		for (auto it = all_sockets.begin(); it != all_sockets.end(); ++it) {
			int fd = *it;
			if (FD_ISSET(fd, &tmp_fds)) {
				if (fd == 0) {
					// citirea de la tastatura
					ret_code = command_from_stdin(all_sockets);

					// daca s-a dat comanda exit se incheie executia programului
					if (ret_code == 0) {
						return 0;
					}
				} else if (fd == sockfd_udp) {
                    /* se parseaza si se trimit mesajele primite de la socket-ul
					UDP clientilor TCP abonati la topicul respectiv */
					process_received_info(fd, clis, topic_subs);
                } else if (fd == sockfd_tcp_listen) {
					add_tcp_client(fdmax, sockfd_tcp_listen, read_fds, to_add,
							sockfd2cli, cli2id, clis, topic_subs, max_cli_id);
				} else {
					/* s-au primit date pe unul din socketii de client,
					asa ca serverul trebuie sa le receptioneze */
					std::vector<std::string> msgs;
					ret_code = get_parsed_messages(fd, msgs);

					int id = sockfd2cli[fd];
					Client& cli = clis[id];

					if (ret_code == -1) {
						// conexiunea s-a inchis
						std::cout << "Client " << cli.name
											   << " disconected." << std::endl;

						/* se scoate din multimea de citire socketul inchis, 
						se elimina clientul din read_fds si se va sterge din
						lista de socketi activi */
						FD_CLR(fd, &read_fds);
						to_delete.push_back(fd);

						// se marcheaza clientul drept fiind offline
						cli.on = false;
						sockfd2cli.erase(fd);

						// closing connection
						shutdown(fd, SHUT_RDWR);
						close(fd);

						continue;
					}

					/* pentru fiecare mesaj de la client (parsat cu ajutorul
					get_parsed_messages) se proceseaza request-ul */
					for (std::string& str : msgs) {
						// conversie din std::string in char *
						char* char_str = (char*) malloc(str.size() + 1);
						DIE(char_str == NULL, "malloc");
						
						memcpy(char_str, str.c_str(), str.size() + 1);
						char_str[str.size()] = '\0';

						process_tcp_client_request(fd, char_str,
												sockfd2cli, clis, topic_subs);
						free(char_str);
					}
				}
			}
		}

		// se elimina din all_sockets conexiunile care au fost inchise
		for (int sock : to_delete) {
			all_sockets.erase(sock);
		}

		// se adauga in all_sockets conexiunile noi
		for (int sock : to_add) {
			all_sockets.insert(sock);
		}
	}

	return 0;
}
