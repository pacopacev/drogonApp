#include <iostream>
#include <string>


class GlobalModel {
public:
    static void initialize() {
        std::cout << "GlobalModel initialized." << std::endl;
    }
    static char toUpper(const std::string& str) {
        if (str.empty()) return '\0';
        return static_cast<char>(std::toupper(str[0]));
    }
    static void shutdown() {
        std::cout << "GlobalModel shutdown." << std::endl;
    }
};