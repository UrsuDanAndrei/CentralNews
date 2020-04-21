#include <iostream>
#include <string>
#include <stdlib.h>
#include <stdio.h>

struct format {
	int len;
	char content[1600];
};



int main() {
	char buffer[1600];
	format *msg = (format*) malloc(sizeof(format));
	std::cout << sizeof(msg) << " " << sizeof(format) << " " << sizeof(buffer) << std::endl;
	return 0;
}