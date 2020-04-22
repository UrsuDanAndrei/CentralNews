#include "tcp_client_handler.h"

void add_tcp_client(int& fdmax, int sockfd_tcp_listen, fd_set &read_fds,
							std::vector<int> &to_add,
							std::unordered_map<int, int> &sockfd2cli,
							std::unordered_map<std::string, int> &cli2id,
							std::vector<Client> &clis,
							int &max_cli_id) {
	printf("in add_tcp_client\n");

	int ret_code;

	// buffer utilizat pentru citire
	char buffer[BUFF_SIZE];
    memset(buffer, 0, BUFF_SIZE);

	/* a venit o cerere de conexiune pe socketul inactiv (cel cu listen),
	pe care serverul o accepta */
	struct sockaddr_in cli_addr;
	socklen_t cli_len = sizeof(cli_addr);
	int new_sockfd = accept(sockfd_tcp_listen, (struct sockaddr *) &cli_addr, &cli_len);
	DIE(new_sockfd < 0, "accept");

	// disable NIGLE Algorithm on this socket
	uint32_t disable = 1;
	ret_code = setsockopt(new_sockfd, IPPROTO_TCP, TCP_NODELAY, (char *) &disable, sizeof(uint32_t));
	DIE(ret_code < 0, "setsockopt");

	/* se adauga noul socket intors de accept() la multimea
	descriptorilor de citire */
	FD_SET(new_sockfd, &read_fds);
	fdmax = std::max(fdmax, new_sockfd);
    to_add.push_back(new_sockfd);

    // se primeste primul mesaj cu numele de autentificare (client_id)
	ret_code = recv(new_sockfd, buffer, BUFF_SIZE, 0);
	DIE(ret_code < 0, "recv");
	int recv_info_size = ret_code;

	format* msg = (format*) buffer;
	printf("New client %s connected from %s:%d.\n",
		msg->content, inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));

	// converteste char* in std::string
	std::string name(msg->content);
	if (cli2id.find(name) == cli2id.end()) {
		/* daca clientul nu s-a mai conectat pana acum atunci i se atribuie un
		id si va ramane inregistrat in server indiferent daca clientul este
		on sau off */
		printf("clientul ");
		std::cout << name << " adaugat in if" <<std::endl;
		cli2id[name] = max_cli_id;
		
		// se realizeaza corespondenta socket client - id client
		sockfd2cli[new_sockfd] = max_cli_id;
		clis.push_back(Client(max_cli_id, new_sockfd , true, name));
		++max_cli_id;
	} else {
		/* daca clientul este deja inregistrat, acesta va incepe sa primeasca
		mesajele din inbox */
		int id = cli2id[name];
		clis[id].on = true;
		clis[id].sockfd = new_sockfd;
		sockfd2cli[new_sockfd] = id;

		std::vector<format*> &inbox = clis[id].inbox;
		printf("incep sa trimit din inbox pentru clientul: ");
		std::cout << clis[id].name << std::endl;
		for (auto& inbox_msg : inbox) {
			printf("trimit\n");
			send(clis[id].sockfd, inbox_msg, inbox_msg->len, 0);
			// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!11 fara free
			// odata trimis, mesajul poate fi eliberat din memoria server-ului
			free(inbox_msg);
		//	usleep(100);
		}

		inbox.clear();
	}

	// daca pe langa nume mai trimite si o cerere
	// int msg_offset = msg->len;
	// while (msg_offset < recv_info_size) {
	// 	msg = (format *) (buffer + msg_offset);
	// 	process_tcp_client_request();
	// 	msg_offset += msg->len;
	// }
}

void process_tcp_client_request(int sockfd, fd_set& read_fds, char *request,
		std::vector<int> &to_delete,
		std::unordered_map<int, int> &sockfd2cli,
		std::vector<Client> &clis,
		std::unordered_map<std::string, std::unordered_set<int>> &topic_subs) {
	int id = sockfd2cli[sockfd];
	Client& cli = clis[id];

//	std::cout << "Client name is: " << cli.name << " and id is: " << id << std::endl;
		printf ("S-a primit de la clientul de pe socketul %d mesajul: %s\n", sockfd, request);

	// !!! strncmp, poate introduci mesaje de verificare pentru
	// !!! a fi sigur ca primesti un input bun de la client, ai avut probleme aici
	char *sub_unsub = strtok(request, " \n");
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
			return;
		}

		std::unordered_set<int> &subs = topic_subs[str_topic];
		if (subs.find(id) == subs.end()) {
			printf("nu exista clientul %d in topic_subs\n", id);
			return;
		}

		subs.erase(id);
		cli.topic_sf.erase(str_topic);
	}
}
