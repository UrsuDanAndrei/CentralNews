#ifndef __SERVER_H__
#define __SERVER_H__

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

/* citeste de la tastatura si executa comanda daca este valida, altfel o ignora,
returneaza 0, daca s-a primit comanda exit */
int command_from_stdin(std::unordered_set<int> &all_sockets);

/* se apeleaza in cazul trimiterii unui numar incorect de parametri */
void usage(char *file);

#endif // __SERVER_H__