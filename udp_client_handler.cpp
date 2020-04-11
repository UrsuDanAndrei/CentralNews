#include "udp_client_handler.h"

void process_received_info(int sockfd, std::vector<Client> &clis,
		std::unordered_map<std::string, std::unordered_set<int>> &topic_subs) {
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
	// converteste din char* in std::string
	// in buffer primul lucru se afla topicul discutiei
	send_to_all_subscribers(buffer, msg, clis, topic_subs);
}

void send_to_all_subscribers(const char *topic, const char *msg,
		std::vector<Client> &clis,
		std::unordered_map<std::string, std::unordered_set<int>> &topic_subs) {
	std::string str_topic(topic);
	std::string str_msg(msg);
	printf("din send_to_all_subscribers\n");
	int ret_code;

	// daca este prima oara cand se face referire la acest topic, inseamna ca niciun
	// client nu este abonat inca
	if (topic_subs.find(str_topic) == topic_subs.end()) {
		printf("nou topic din send_all_subscribers\n");
		// topic_subs[str_topic] = std::unordered_set<int>();
		return;
	}

	std::unordered_set<int>& subs = topic_subs[str_topic];
	for (auto it = subs.begin(); it != subs.end(); ++it) {
		int id = *it;
		Client& cli = clis[id];

		if (cli.on) {
			// poate trebuie alte falg-uri
			printf("Trimit catre clientul: ");
			std::cout << cli.name << std::endl;
			ret_code = send(cli.sockfd, msg, strlen(msg) + 1, 0);
			DIE(ret_code < 0, "send");
		} else {
			if (cli.topic_sf[str_topic] == true) {
				printf("Adaug in inbox pentru: ");
				std::cout << cli.name << std::endl;
				cli.inbox.push_back(str_msg);
			}
		}
	} 
}
