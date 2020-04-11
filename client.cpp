#include "client.h"

Client::Client(int id, int sockfd, bool on, std::string name) {
	this->id = id;
	this->sockfd = sockfd;
	this->on = on;
	this->name = name;
}
