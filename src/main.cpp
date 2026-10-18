

#include "alma.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>

void showUsage()
{
    std::string message;
    message = R"END(
 ALMA  Copyright (C) 2026  Héctor Galbis Sanchis

 This is the ALMA interpreter. This is for now a proof of concept.

 Usage:

   alma2c input -o output
 )END";

    std::cout << message << std::endl;
}

int main(int argc, char* argv[])
{
    std::optional<std::string> input;
    std::optional<std::string> output;
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "-o" && i + 1 < argc && !output)
            output = argv[++i];
        else if (arg != "-o" && !input)
            input = arg;
        else {
            input.reset();
            break;
        }
    }

    if (!input || !output) {
        std::cerr << "Expected an input file and an output file" << std::endl;
        showUsage();
        exit(1);
    }

    try {
        Alma::alma.load(*input);
        std::ofstream file(*output);
        if (!file) {
            std::cerr << "Cannot open the output file " << *output << std::endl;
            return 1;
        }
        file << Alma::alma.get_output().str();
    } catch (std::runtime_error& e) {
        std::string message = e.what();
        message.erase(message.find_last_not_of('\n') + 1);
        std::cerr << message << std::endl;
        return 1;
    }

    return 0;
}
