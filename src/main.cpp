

#include "alma.hpp"
#include "object/function.hpp"
#include "reader.hpp"
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <spawn.h>
#include <sys/wait.h>
#include <vector>

extern char** environ;

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

static int compile(const std::optional<std::string>& name)
{
    std::string dir_template = (std::filesystem::temp_directory_path() / "alma-XXXXXX").native();
    if (!mkdtemp(dir_template.data())) {
        std::cerr << "Cannot create a temporary directory" << std::endl;
        return 1;
    }
    std::filesystem::path dir = dir_template;
    std::filesystem::path c_file = dir / "repl.c";
    {
        std::ofstream file(c_file);
        if (!file) {
            std::cerr << "Cannot open the output file " << c_file.native() << std::endl;
            std::filesystem::remove_all(dir);
            return 1;
        }
        file << Alma::alma.get_c_code();
    }

    std::vector<std::string> args = { "gcc", c_file.native() };
    if (name) {
        args.push_back("-o");
        args.push_back(*name);
    }
    std::vector<char*> argv;
    for (std::string& arg : args)
        argv.push_back(arg.data());
    argv.push_back(nullptr);

    int status = 1;
    pid_t pid;
    if (posix_spawnp(&pid, "gcc", nullptr, nullptr, argv.data(), environ) != 0)
        std::cerr << "Cannot run gcc" << std::endl;
    else if (waitpid(pid, &status, 0) == -1)
        status = 1;
    else
        status = WIFEXITED(status) ? WEXITSTATUS(status) : 1;

    std::filesystem::remove_all(dir);
    return status;
}

static int repl()
{
    while (true) {
        std::cout << "alma> " << std::flush;
        try {
            Reader reader(Alma::alma, "<repl>", std::cin);
            std::optional<ObjectRef<Object>> obj = reader.read();
            if (!obj) {
                std::cout << std::endl;
                return 0;
            }
            std::cout << Alma::alma.to_string(Alma::alma.eval(*obj)) << std::endl;
        } catch (const ExitRequest& e) {
            return e.generate ? compile(e.name) : 0;
        } catch (const std::runtime_error& e) {
            std::string message = e.what();
            message.erase(message.find_last_not_of('\n') + 1);
            std::cerr << message << std::endl;
            if (std::cin.eof()) {
                std::cout << std::endl;
                return 0;
            }
            std::cin.clear();
        }
    }
}

int main(int argc, char* argv[])
{
    if (argc == 1)
        return repl();

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
        try {
            Alma::alma.load(*input);
        } catch (const ExitRequest& e) {
            if (!e.generate)
                return 1;
        }
        std::ofstream file(*output);
        if (!file) {
            std::cerr << "Cannot open the output file " << *output << std::endl;
            return 1;
        }
        file << Alma::alma.get_c_code();
    } catch (std::runtime_error& e) {
        std::string message = e.what();
        message.erase(message.find_last_not_of('\n') + 1);
        std::cerr << message << std::endl;
        return 1;
    }

    return 0;
}
