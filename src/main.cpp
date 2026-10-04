

#include "alma.hpp"
#include <filesystem>
#include <iostream>

void showUsage()
{
    std::string message;
    message = R"END(
 ALMA  Copyright (C) 2026  Héctor Galbis Sanchis

 This is the ALMA interpreter. This is for now a proof of concept.

 Usage:

   alma input
 )END";

    std::cout << message << std::endl;
}

int main(int argc, char* argv[])
{
    if (argc != 2) {
        std::cerr << "Expected 2 arguments" << std::endl;
        showUsage();
        exit(1);
    }

    try {
        Alma::alma.load(argv[1]);
    } catch (std::runtime_error& e) {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}
