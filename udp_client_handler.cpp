#include "udp_client_handler.h"

void process_received_info(int sockfd, std::vector<Client> &clis,
		std::unordered_map<std::string, std::unordered_set<int>> &topic_subs) {
	// buffer pentru citirea mesajelor primite
	char buffer[BUFF_SIZE];
	memset(buffer, 0, BUFF_SIZE);


	struct sockaddr_in client_addr;
	socklen_t client_len = sizeof(struct sockaddr_in);
	memset(&client_addr, 0, sizeof(struct sockaddr_in));

	// se salveaza in buffer informatia primita de pe socket-ul UDP
	recvfrom(sockfd, buffer, sizeof(buffer), MSG_WAITALL,
								(struct sockaddr*) &client_addr, &client_len);
	
	// informatia din buffer va fi parsata in msg
	format *msg = (format*) malloc(sizeof(format));
	DIE(msg == NULL, "malloc");
	memset(msg, 0, sizeof(format));
	
	// IP
	char *ip_addr = inet_ntoa(client_addr.sin_addr);
	memcpy(msg->content, ip_addr, strlen(ip_addr));

	// IP:
	int pct2_offset = strlen(ip_addr);
	msg->content[pct2_offset] = ':';

	// IP:PORT
	int port_offset = pct2_offset + 1;
	int port_no_len = sprintf(msg->content + port_offset, "%d",
												ntohs(client_addr.sin_port));

	// IP:PORT - 
	int s_line_s_offset = port_offset + port_no_len;
	msg->content[s_line_s_offset] = ' ';
	msg->content[s_line_s_offset + 1] = '-';
	msg->content[s_line_s_offset + 2] = ' ';

	// se gestioneaza cazul in care topicul are exact 50 de caractere
	char saved_char = buffer[MAX_TOPIC_LEN];
	buffer[MAX_TOPIC_LEN] = '\0';

	/* strlen returneaza dimensiunea pana la primul '\0' din buffer, adica exact
	lungimea topicului */

	// IP:PORT - topic
	int topic_offset = s_line_s_offset + 3;
	memcpy(msg->content + topic_offset, buffer, strlen(buffer));

	buffer[MAX_TOPIC_LEN] = saved_char;
	
	// IP:PORT - topic - 
	int s_line_s_offset2 = topic_offset + strlen(buffer);
	msg->content[s_line_s_offset2] = ' ';
	msg->content[s_line_s_offset2 + 1] = '-';
	msg->content[s_line_s_offset2 + 2] = ' ';

	// IP:PORT - topic - tip_date - valoare mesaj
	int type_offset =  s_line_s_offset2 + 3;
	uint8_t type;
	memcpy(&type, buffer + TYPE_OFFSET, 1);

	// in functie de tipul primit parsarea se realizeaza diferentiat
	switch (type) {
		case INT: {
			msg->content[type_offset] = 'I';
			msg->content[type_offset + 1] = 'N';
			msg->content[type_offset + 2] = 'T';

			int s_line_s_offset3 = type_offset + 3;
			msg->content[s_line_s_offset3] = ' ';
			msg->content[s_line_s_offset3 + 1] = '-';
			msg->content[s_line_s_offset3 + 2] = ' ';

			uint8_t sign;
			memcpy(&sign, buffer + SIGN_OFFSET, 1);

			uint32_t no;
			memcpy(&no, buffer + INT_OFFSET, sizeof(uint32_t));
			no = ntohl(no);

			/* daca se este necesar se adauga semnul '-' si se deplaseaza 
			numarul la dreapta */
			int no_offset = s_line_s_offset3 + 3;
			if (sign == 1) {
				msg->content[no_offset] = '-';
				++no_offset;
			}

			// se plaseaza numarul in msg
			int no_len = sprintf(msg->content + no_offset, "%d", no);

			int final_len = no_offset + no_len;
			msg->content[final_len] = '\0';

			break;
		}
		
		case SHORT_REAL: {
			msg->content[type_offset] = 'S';
			msg->content[type_offset + 1] = 'H';
			msg->content[type_offset + 2] = 'O';
			msg->content[type_offset + 3] = 'R';
			msg->content[type_offset + 4] = 'T';
			msg->content[type_offset + 5] = '_';
			msg->content[type_offset + 6] = 'R';
			msg->content[type_offset + 7] = 'E';
			msg->content[type_offset + 8] = 'A';
			msg->content[type_offset + 9] = 'L';

			int s_line_s_offset3 = type_offset + 10;
			msg->content[s_line_s_offset3] = ' ';
			msg->content[s_line_s_offset3 + 1] = '-';
			msg->content[s_line_s_offset3 + 2] = ' ';

			uint16_t no;
			memcpy(&no, buffer + SHORT_REAL_OFFSET, sizeof(uint16_t));
			no = ntohs(no);

			int no_offset = s_line_s_offset3 + 3;
			int no_len = sprintf(msg->content + no_offset, "%d", no);

			/* se muta ultimele 2 cifre cu o pozitie la dreapta pentru a face
			loc caracterului '.' */
			memcpy(msg->content + no_offset + no_len - 1,
									msg->content + no_offset + no_len - 2, 2);

			// se plaseaza caracterul '.'
			int dot_offset = no_offset + no_len - 2;
			msg->content[dot_offset] = '.';

			int final_len = no_offset + no_len + 1;
			msg->content[final_len] = '\0';

			break;
		}

		case FLOAT: {
			msg->content[type_offset] = 'F';
			msg->content[type_offset + 1] = 'L';
			msg->content[type_offset + 2] = 'O';
			msg->content[type_offset + 3] = 'A';
			msg->content[type_offset + 4] = 'T';

			int s_line_s_offset3 = type_offset + 5;
			msg->content[s_line_s_offset3] = ' ';
			msg->content[s_line_s_offset3 + 1] = '-';
			msg->content[s_line_s_offset3 + 2] = ' ';

			uint8_t sign;
			memcpy(&sign, buffer + SIGN_OFFSET, sizeof(uint8_t));

			/* daca se este necesar se adauga semnul '-' si se deplaseaza 
			numarul la dreapta */
			int no_offset = s_line_s_offset3 + 3;
			if (sign == 1) {
				msg->content[no_offset] = '-';
				++no_offset;
			}

			uint32_t no;
			memcpy(&no, buffer + FLOAT_OFFSET, sizeof(uint32_t));
			no = ntohl(no);
			int no_len = sprintf(msg->content + no_offset, "%d", no);

			uint8_t power10;
			memcpy(&power10, buffer + POWER10_OFFSET, sizeof(uint8_t));

			/* se introduc numarul de zerouri necesare pentru o
			reprezentare corecta a numarului */
			int no_zeros = power10 - no_len + 1;
			if (no_zeros > 0) {
				/* se deplaseaza numarul cu nr_zeros la dreapta pentru a face
				loc  pentru zero-urile ce urmeaza a fi adaugate */
				memcpy(msg->content + no_offset + no_zeros,
											msg->content + no_offset, no_len);
				no_len += no_zeros;
				
				// se adauga zero-urile
				while (no_zeros != 0) {
					msg->content[no_offset + no_zeros - 1] = '0';
					--no_zeros;
				}
			}
			
			if (power10 != 0) {
				// se plaseaza caracterul '.'
				memcpy(msg->content + no_offset + no_len - (power10 - 1),
						msg->content + no_offset + no_len - power10, power10);

				int dot_offset = no_offset + no_len - power10;
				msg->content[dot_offset] = '.';

				int final_len = no_offset + no_len + 1;
				msg->content[final_len] = '\0';
			}

			break;
		}

		case STRING: {
			msg->content[type_offset] = 'S';
			msg->content[type_offset + 1] = 'T';
			msg->content[type_offset + 2] = 'R';
			msg->content[type_offset + 3] = 'I';
			msg->content[type_offset + 4] = 'N';
			msg->content[type_offset + 5] = 'G';

			int s_line_s_offset3 = type_offset + 6;
			msg->content[s_line_s_offset3] = ' ';
			msg->content[s_line_s_offset3 + 1] = '-';
			msg->content[s_line_s_offset3 + 2] = ' ';
 
			int string_offset = s_line_s_offset3 + 3;
			strncpy(msg->content + string_offset, buffer + STRING_OFFSET,
															MAX_STRING_SIZE);

			break;
		}
	}

	// 2 este pentru cei 2 bytes din primul int, 1 este pentru '\0' din coada
	msg->len = 2 + strlen(msg->content) + 1;

	// in buffer primul string este reprezentat de topicul discutiei
	buffer[MAX_TOPIC_LEN] = '\0';
	send_to_all_subscribers(buffer, msg, clis, topic_subs);
}

void send_to_all_subscribers(const char *topic, format *msg,
		std::vector<Client> &clis,
		std::unordered_map<std::string, std::unordered_set<int>> &topic_subs) {
	std::string str_topic(topic);

	int ret_code;

	/* daca este prima oara cand se face referire la acest topic, inseamna ca
	niciun client nu este abonat inca, deci se da drop la mesaj */
	if (topic_subs.find(str_topic) == topic_subs.end()) {
		free(msg);
		return;
	}

	// se parcurge lista de subscriberi ai topicului
	std::unordered_set<int>& subs = topic_subs[str_topic];
	for (auto it = subs.begin(); it != subs.end(); ++it) {
		int id = *it;
		Client& cli = clis[id];
		
		// daca clientul este on se trimite pur si simplu mesajul
		if (cli.on) {
			ret_code = send(cli.sockfd, msg, msg->len, 0);
			DIE(ret_code < 0, "send");
		} else {
			// altfel se adauga o copie a acestui in inbox-ul sau
			if (cli.topic_sf[str_topic] == true) {
				format* copy_msg = (format *) malloc(sizeof(format));
				DIE(copy_msg == NULL, "malloc");

				memcpy(copy_msg, msg, sizeof(format));
				cli.inbox.push_back(copy_msg);
			}
		}
	}

	// se elibereaza memoria alocata mesajului
	free(msg);
}
