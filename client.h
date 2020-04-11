#ifndef __CLIENT_H__
#define __CLIENT_H__

#include <string>
#include <vector>
#include <unordered_map>

class Client {
 public:
	int id;
	int sockfd;
	bool on;

	std::string name;
	std::vector<std::string> inbox;
	std::unordered_map<std::string, bool> topic_sf;


	Client(int id, int sockfd, bool on, std::string name);
};

#endif // __CLIENT_H__