#include <iostream>
#include "Manager.hpp"
#include "Computation.hpp"


int main(int argc, char *argv[])
{
    if (argc == 1) {
        while (true) {
            Manager m(argv[0], 4);
            m.run();
            std::cout << "\n";
        }
    }
    else {
        std::string argvStrings[] = {argv[0], argv[1], argv[2]};
        if (argvStrings[1] == "f" || argvStrings[1] == "g") {
            Computation c(argvStrings[1][0], std::stoi(argvStrings[2]));
            c.run();
        }
        else {
            std::cerr << "Unexpected command line arguments.";
            return 1;
        }
    }
    return 0;
}
