#ifndef __UTILS_H__
#define __UTILS_H__

#include <stdio.h>
#include <stdlib.h>
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

#define DIE(assertion, call_description)				\
	do {								\
		if (assertion) {					\
			fprintf(stderr, "(%s, %d): ",			\
					__FILE__, __LINE__);		\
			perror(call_description);			\
			exit(EXIT_FAILURE);				\
		}							\
	} while(0)

#define BUFF_SIZE 1600
#define MAX_CLIENTS 1000

#define MAX_TOPIC_LEN 50

#define COMMAND_LEN 5

#define INT 0
#define INT_OFFSET 52

#define SHORT_REAL 1
#define SHORT_REAL_OFFSET 51

#define FLOAT 2
#define FLOAT_OFFSET 52
#define POWER10_OFFSET 56

#define STRING 3
#define STRING_OFFSET 51
#define MAX_STRING_SIZE 1500
#define TYPE_OFFSET 50
#define SIGN_OFFSET 51

/* mesajele vor fi structurate in functie de structura de mai jos */
struct format {
	uint16_t len;
	char content[BUFF_SIZE];
} __attribute__((packed));

/* primeste un socket si returneaza prin parametrul msg toate mesajele primite
pe acel socket la un anumit moment de timp (desparte mesajele unite de TCP si
uneste mesajele despartite de TCP), returneaza -1 daca s-a trimis un mesaj de
inchidere a conexiunii pe socket-ul primit, altfel returneaza dimensiunea
vectorului msg */
int get_parsed_messages(int sockfd, std::vector<std::string>& msgs);

#endif // __HELPERS_H__
