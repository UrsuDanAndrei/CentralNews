#ifndef __SUBSCRIBER_H__
#define __SUBSCRIBER_H__

#include "utils.h"

#include <arpa/inet.h>
#include <bits/stdc++.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

/* se apeleaza in cazul trimiterii unui numar incorect de parametri */
void usage(char *file);

// afiseaza un mesaj de eroare
void wrong_command();

/* returneaza -1 daca inputul este invalid, 0 daca s-a dat comanda exit si
1 daca totul este ok */
int check_correct_input_subscriber(const char *input);

#endif // __SUBSCRIBER_H__