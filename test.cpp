#include <iostream>
#include <string>

int main() {
	std::string s("ab\0cd");
	std::cout << s << std::endl;
	return 0;
}