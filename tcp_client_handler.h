#ifndef __TCP_CLIENT_HANDLER_H__
#define __TCP_CLIENT_HANDLER_H__

#include "client.h"
#include "utils.h"

#include <arpa/inet.h>
#include <bits/stdc++.h>
#include <iostream>
#include <netinet/tcp.h>
#include <stdio.h>
#include <string>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>

/* primeste un request de tipul subscribe/unsubscribe si updateaza structurile
de date primite ca parametru pentru a corespunde cu rezultatul request-ului */
void process_tcp_client_request(int sockfd, char *request,
		std::unordered_map<int, int> &sockfd2cli,
		std::vector<Client> &clis,
		std::unordered_map<std::string, std::unordered_set<int>> &topic_subs);

/* inregistreaza un nou client sau reconecteaza un client vechi (in cazul
clientului vechi, ii trimite toate mesajele salvate in inbox-ul acestuia) */
void add_tcp_client(int& fdmax, int sockfd_tcp_listen, fd_set &read_fds,
		std::vector<int> &to_add,
		std::unordered_map<int, int> &sockfd2cli,
		std::unordered_map<std::string, int> &cli2id,
		std::vector<Client> &clis, 
		std::unordered_map<std::string, std::unordered_set<int>> &topic_subs,
		int &max_cli_id);

#endif // __TCP_CLIENT_HANDLER_H__