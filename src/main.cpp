#include <iostream>
#include "module-test/test.h"

int main() {
    std::cout << "Hello World" << std::endl;
    int a = test();
    std::cout << a << std::endl;
    std::cout << "I changed the docker" << std::endl;
    return 0;
}
