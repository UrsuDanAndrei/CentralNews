#ifndef __UDP_CLIENT_HANDLER_H__
#define __UDP_CLIENT_HANDLER_H__

#include "client.h"
#include "utils.h"

#include <arpa/inet.h>
#include <bits/stdc++.h>
#include <iostream>
#include <stdio.h>
#include <string>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>

void process_received_info(int sockfd, std::vector<Client> &clis,
		std::unordered_map<std::string, std::unordered_set<int>> &topic_subs);

void send_to_all_subscribers(const char *topic, format *msg,
		std::vector<Client> &clis,
		std::unordered_map<std::string, std::unordered_set<int>> &topic_subs);

#endif // __UDP_CLIENT_HANDLER_H__