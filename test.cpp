#include <iostream>
#include <string>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct format {
	uint16_t len;
	char content[1600];
};



int main() {
	// char buffer[1600];
	// format *msg = (format*) malloc(sizeof(format));
	// std::cout << sizeof(msg) << " " << sizeof(format) << " " << sizeof(buffer) << std::endl;
	std::cout << sizeof(struct format) << std::endl;
	std::string str("ceva");
	char* chs = (char*) malloc(str.size() + 1);
	memcpy(chs, str.c_str(), 5);
	std::cout << chs << std::endl;
	std::cout << sizeof(str.c_str()) << std::endl;
	if (chs[4] == '\0') {
		std::cout << "ieeeeeee\n";
	}

	uint16_t x = 0;;
	char ccc[10];
	fgets(ccc, 10, stdin);
	ccc[0] -= '0';
	memcpy(&x, ccc, 1);
	fgets(ccc + 1, 9, stdin);
	ccc[1] -= '0';
	memcpy(((&x) + 1), ccc + 1, 1);
	std::cout<<x<<std::endl;

	std::cout << &x << std::endl;
	std::cout << ((&x) + 1) << std::endl;

	return 0;
}