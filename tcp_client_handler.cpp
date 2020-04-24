#include "tcp_client_handler.h"

void add_tcp_client(int& fdmax, int sockfd_tcp_listen, fd_set &read_fds,
		std::vector<int> &to_add,
		std::unordered_map<int, int> &sockfd2cli,
		std::unordered_map<std::string, int> &cli2id,
		std::vector<Client> &clis,
		std::unordered_map<std::string, std::unordered_set<int>> &topic_subs,
		int &max_cli_id) {
	int ret_code;

	/* a venit o cerere de conexiune pe socketul inactiv (cel cu listen),
	pe care serverul o accepta */
	struct sockaddr_in cli_addr;
	socklen_t cli_len = sizeof(cli_addr);
	int new_sockfd = accept(sockfd_tcp_listen,
									(struct sockaddr *) &cli_addr, &cli_len);
	DIE(new_sockfd < 0, "accept");

	// disable NEAGLE Algorithm on this socket
	uint32_t disable = 1;
	ret_code = setsockopt(new_sockfd, IPPROTO_TCP, TCP_NODELAY,
										(char *) &disable, sizeof(uint32_t));
	DIE(ret_code < 0, "setsockopt");

	/* se adauga noul socket intors de accept() la multimea
	descriptorilor de citire */
	FD_SET(new_sockfd, &read_fds);
	fdmax = std::max(fdmax, new_sockfd);
    to_add.push_back(new_sockfd);

	std::vector<std::string> msgs;
	ret_code = get_parsed_messages(new_sockfd, msgs);

	/* daca clientul nu isi trimite macar numele atunci socket-ul a aparut o
	eroare si se inchide socket-ul */
	if (ret_code == 0) {
		// std::cout << "Clientul nu a specificat numele de autentificare\n";
		FD_CLR(new_sockfd, &read_fds);

		shutdown(new_sockfd, SHUT_RDWR);
		close(new_sockfd);

		to_add.pop_back();
		return;
	}

    // se proceseaza primul mesaj cu numele de autentificare (client_id)
	std::string name = msgs[0];
	if (cli2id.find(name) == cli2id.end()) {
		/* daca clientul nu s-a mai conectat pana acum atunci i se atribuie un
		id si va ramane inregistrat in server indiferent daca clientul este
		on sau off */
		std::cout << "New client " << name << " connected from "
				  << inet_ntoa(cli_addr.sin_addr) << ":"
				  << ntohs(cli_addr.sin_port) << "." << std::endl;
		cli2id[name] = max_cli_id;
		
		// se realizeaza corespondenta socket client - id client
		sockfd2cli[new_sockfd] = max_cli_id;
		clis.push_back(Client(max_cli_id, new_sockfd , true, name));
		++max_cli_id;
	} else {
		/* daca clientul este deja logat se inchide socket-ul corespunzator
		celei de a 2 incercari de logare */
		int id = cli2id[name];
		if (clis[id].on) {
			// std::cout << "Clientul este deja logat\n";
			FD_CLR(new_sockfd, &read_fds);

			shutdown(new_sockfd, SHUT_RDWR);
			close(new_sockfd);

			to_add.pop_back();
			return;
		}

		std::cout << "New client " << name << " connected from "
				  << inet_ntoa(cli_addr.sin_addr) << ":"
				  << ntohs(cli_addr.sin_port) << "." << std::endl;

		/* daca clientul este deja inregistrat, acesta va incepe sa primeasca
		mesajele din inbox */
		clis[id].on = true;
		clis[id].sockfd = new_sockfd;
		sockfd2cli[new_sockfd] = id;

		std::vector<format*> &inbox = clis[id].inbox;
		for (auto& inbox_msg : inbox) {
			send(clis[id].sockfd, inbox_msg, inbox_msg->len, 0);
			free(inbox_msg);
		}

		inbox.clear();
	}

	// poate pe langa nume, clientul a trimis unul sau mai multe request-uri
	for (int i = 1; i < (int) msgs.size(); ++i) {
		// conversie din std::string in char*
		std::string& str = msgs[i];
		char* char_str = (char*) malloc(str.size() + 1);
		DIE(char_str == NULL, "malloc");

		memcpy(char_str, str.c_str(), str.size() + 1);
		char_str[str.size()] = '\0';

		process_tcp_client_request(new_sockfd, char_str,
								sockfd2cli, clis, topic_subs);
		free(char_str);
	}
}

void process_tcp_client_request(int sockfd, char *request,
		std::unordered_map<int, int> &sockfd2cli,
		std::vector<Client> &clis,
		std::unordered_map<std::string, std::unordered_set<int>> &topic_subs) {
	// informatii legate de client
	int id = sockfd2cli[sockfd];
	Client& cli = clis[id];

	// se verifica tipul mesajului primit
	char *sub_unsub = strtok(request, " \n");
	if (sub_unsub == NULL) {
		// std::cout << "Mesajul nu respecta formatul dorit\n";
		return;
	}

	char *topic = strtok(NULL, " \n");
	if (topic == NULL) {
		// std::cout << "Mesajul nu respecta formatul dorit\n";
		return;
	}

	std::string str_topic(topic);

	if (strncmp(sub_unsub, "subscribe", 9) == 0) {
		// daca se primeste o cerere de subscribe se extrage flag-ul de sf
		char *sf = strtok(NULL, " \n");
		if (sf == NULL) {
			// std::cout << "Mesajul nu respecta formatul dorit\n";
			return;
		}

		/* se adauga clientul la lista de abonati a topicului (daca topicul este
		unul nou, atunci se creeaza aceasta lista) */ 
		if (topic_subs.find(str_topic) == topic_subs.end()) {
			topic_subs[str_topic] = std::unordered_set<int>();
			topic_subs[str_topic].insert(id);
		} else {
			std::unordered_set<int>& subs = topic_subs[str_topic];
			if (subs.find(id) == subs.end()) {
				subs.insert(id);
			}

			/* daca clientul este deja subscribed la acest topic, atunci
			aici se poate trimite un mesaj sa il informeze de acest lucru,
			eu am ales pur si simplu sa ii permite clientului sa se aboneze
			cu un alt sf flag */
		}

		/* se introduce topicul in lista de topicuri a clientului si se 
		marcheaza cu true pentru flag-ul sf setat pe 1, false altfel */
		if (sf[0] == '0') {
			cli.topic_sf[str_topic] = false;
		} else if (sf[0] == '1') {
			cli.topic_sf[str_topic] = true;
		} else {
			// std::cout << "Mesajul nu respecta formatul dorit\n";
		}
	} else if (strncmp(sub_unsub, "unsubscribe", 11) == 0) {
		/* daca clinet-ul vrea sa isi dea unsubscribe la un topic inexistent
		se ignora pur si simplu mesajul */
		if (topic_subs.find(str_topic) == topic_subs.end()) {
			// std::cout << "Nu exista acest topic\n";
			return;
		}

		/* daca clinet-ul vrea sa isi dea unsubscribe la un topic la care nu
		este abonat se ignora pur si simplu mesajul */
		std::unordered_set<int> &subs = topic_subs[str_topic];
		if (subs.find(id) == subs.end()) {
			// std::cout << "Clientul nu este inregistrat la acest topic\n";
			return;
		}

		// se elimina clientul din lista subscriberilor pentru topicul primit
		subs.erase(id);

		// se elimina topicul din lista topicurilor clientului
		cli.topic_sf.erase(str_topic);
	} else {
		// se ignora mesajele care nu respecta formatul din if-uri
		// std::cout << "Mesajul nu respecta formatul dorit\n";
	}
}
