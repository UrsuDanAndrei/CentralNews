#include <iostream>
#include <string>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct format {
	int len;
	char content[1600];
};



int main() {
	// char buffer[1600];
	// format *msg = (format*) malloc(sizeof(format));
	// std::cout << sizeof(msg) << " " << sizeof(format) << " " << sizeof(buffer) << std::endl;

	std::string str("ceva");
	char* chs = (char*) malloc(str.size() + 1);
	memcpy(chs, str.c_str(), 5);
	std::cout << chs << std::endl;
	std::cout << sizeof(str.c_str()) << std::endl;
	if (chs[4] == '\0') {
		std::cout << "ieeeeeee\n";
	}
	return 0;
}