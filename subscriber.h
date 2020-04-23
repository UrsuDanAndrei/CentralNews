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

void usage(char *file);

void wrong_command();

int check_correct_input_subscriber(const char *input);

#endif // __SUBSCRIBER_H__