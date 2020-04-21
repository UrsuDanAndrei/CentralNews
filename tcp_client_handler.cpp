#include "tcp_client_handler.h"

void add_tcp_client(int& fdmax, int sockfd_tcp_listen, fd_set &read_fds,
							std::vector<int> &to_add,
							std::unordered_map<int, int> &sockfd2cli,
							std::unordered_map<std::string, int> &cli2id,
							std::vector<Client> &clis,
							int &max_cli_id) {
	printf("in add_tcp_client\n");

	int ret_code;
	char buffer[BUFF_SIZE];
    memset(buffer, 0, sizeof(buffer));

	// a venit o cerere de conexiune pe socketul inactiv (cel cu listen),
	// pe care serverul o accepta
	struct sockaddr_in cli_addr;
	socklen_t cli_len = sizeof(cli_addr);
	int new_sockfd = accept(sockfd_tcp_listen, (struct sockaddr *) &cli_addr, &cli_len);
	DIE(new_sockfd < 0, "accept");

	// se adauga noul socket intors de accept() la multimea descriptorilor de citire
	FD_SET(new_sockfd, &read_fds);
	fdmax = std::max(fdmax, new_sockfd);
    to_add.push_back(new_sockfd);

    // se primeste primul mesaj cu numele de autentificare (client_id)
	ret_code = recv(new_sockfd, buffer, sizeof(buffer), 0);
	DIE(ret_code < 0, "recv");

	// !!!!!! s-ar putea sa primeasca si o cerere de unsubscribe / subscribe in acelasi timp
	printf("New client %s connected from %s:%d.\n",
		buffer, inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));

	// converteste char* in std::string
	std::string name(buffer);
	if (cli2id.find(name) == cli2id.end()) {
		printf("clientul ");
		std::cout << name << " adaugat in if" <<std::endl;
		cli2id[name] = max_cli_id;
		sockfd2cli[new_sockfd] = max_cli_id;
		clis.push_back(Client(max_cli_id, new_sockfd , true, name));
		++max_cli_id;
	} else {
		int id = cli2id[name];
		clis[id].on = true;
		clis[id].sockfd = new_sockfd;
		sockfd2cli[new_sockfd] = id;

		std::vector<format*> &inbox = clis[id].inbox;
		printf("incep sa trimit din inbox pentru clientul: ");
		std::cout << clis[id].name << std::endl;
		for (auto& msg : inbox) {
			printf("trimit\n");
			send(clis[id].sockfd, msg, msg->len, 0);
			// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!11 fara free
			free(msg);
			usleep(100);
		}

		inbox.clear();
	}

}

void process_tcp_client_request(int sockfd, fd_set& read_fds,
		std::vector<int> &to_delete,
		std::unordered_map<int, int> &sockfd2cli,
		std::vector<Client> &clis,
		std::unordered_map<std::string, std::unordered_set<int>> &topic_subs) {
	int ret_code;
    char buffer[BUFF_SIZE];
	memset(buffer, 0, sizeof(buffer));

    printf("in process_tcp_client_request\n");
	// s-au primit date pe unul din socketii de client,
	// asa ca serverul trebuie sa le receptioneze
	ret_code = recv(sockfd, buffer, sizeof(buffer), 0);
	DIE(ret_code < 0, "recv");

	int id = sockfd2cli[sockfd];
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
		FD_CLR(sockfd, &read_fds);
		to_delete.push_back(sockfd);

		cli.on = false;
		sockfd2cli.erase(sockfd);

		// closing connection
		shutdown(sockfd, SHUT_RDWR);
		close(sockfd);
	} else {
		printf ("S-a primit de la clientul de pe socketul %d mesajul: %s\n", sockfd, buffer);

		// !!! strncmp, poate introduci mesaje de verificare pentru
		// !!! a fi sigur ca primesti un input bun de la client, ai avut probleme aici
		char *sub_unsub = strtok(buffer, " \n");
		char *topic = strtok(NULL, " \n");
		std::string str_topic(topic);

		if (strncmp(sub_unsub, "subscribe", 9) == 0) {
			char *sf = strtok(NULL, " \n");
			printf ("din subscribe\n");
			cli.topic_sf[str_topic] = true;

			if (topic_subs.find(str_topic) == topic_subs.end()) {
				printf("din if new topic, topicul adaugat este: ");
				std::cout << str_topic << " ";
				printf("clientul abonat este: %d\n", id);
				topic_subs[str_topic] = std::unordered_set<int>();
				topic_subs[str_topic].insert(id);
			} else {
				std::unordered_set<int>& subs = topic_subs[str_topic];
				if (subs.find(id) == subs.end()) {
					subs.insert(id);
				} else {
					// handle error este deja acolo
					// poate vrea sa fie subscribed altfel
				}
			}

			// std::unordered_map<std::string, bool> &topic_sf = cli.topic_sf;
			if (sf[0] == '0') {
				printf("Abonat cu 0\n");
				cli.topic_sf[str_topic] = false;
			} else {
				cli.topic_sf[str_topic] = true;
			}
		} else if (strncmp(sub_unsub, "unsubscribe", 11) == 0) {
			printf("din unsubscribe, topic: ");
			std::cout << str_topic;
			std::cout << "lipita" << std::endl;
			// cazul cu unsubscribe
			// !!! poate testezi cazul cu nu este subscribed
			// !!! poate testezi cazul in care nu exista topic-ul ala
			if (topic_subs.find(str_topic) == topic_subs.end()) {
				printf("Nu exista acest topic\n");
			}

			std::unordered_set<int> &subs = topic_subs[str_topic];

			if (subs.find(id) == subs.end()) {
				printf("nu exista clientul %d in topic_subs\n", id);
			}

			subs.erase(id);
			cli.topic_sf.erase(str_topic);
		}
	}
}
