#include <iostream>
#include "process.h"

int main(int argc, char** argv) {
    std::string command;

    std::cout << "Please enter a command to execute: ";
    std::getline(std::cin, command);

    int exit_code = Process::run(command.c_str());
    if (exit_code == -1) {
        std::cerr << "An error occurred while executing the command." << std::endl;
        return 1;
    }

    std::cout << "Command executed successfully with exit code: " << exit_code << std::endl;
    return 0;
}