//
// Created by Ken on 2023/5/22.
//

#include <iostream>
#include <boost/program_options.hpp>
#include <thread>
#include "device/Socat.h"
#include "UShellCmdInterceptor.h"

bool running = false;

void socatReader(Socat *socat) {
    std::string line;

    while (running) {
        unsigned long read = socat->read(line);
        if (read > 0) {
            std::cout << line;
        }
    }
}

int main(int argc, char **argv) {

    // setup program options description
    boost::program_options::options_description ushellTerminalOptions("UShell Terminal Options");
    ushellTerminalOptions.add_options()
            ("help,h", "Produce help message")
            ("ushell,u", boost::program_options::value<std::string>(),
             "The virtio device UShell lives on (default: /tmp/port0)")
            ("context,c", boost::program_options::value<std::string>(),
             "Path to the directory on the host mounted to UShell (default: ./fs0)")
            ("root,r", boost::program_options::value<std::string>(),
             "Root directory of the UShell context (default: /ushell");
    boost::program_options::variables_map mVMap;

    try {
        // parse program options

        boost::program_options::store(boost::program_options::parse_command_line(argc, argv, ushellTerminalOptions),
                                      mVMap);
        boost::program_options::notify(mVMap);

        if (mVMap.count("help")) {
            std::cout << ushellTerminalOptions << std::endl;
            return 0;
        }
    } catch (const std::exception &exception) {
        std::cerr << "Invalid Usage: " << exception.what() << "\n";
        std::cerr << ushellTerminalOptions << std::endl;
        return -1;
    }

    // create socat device
    std::string ushellDevicePath = "/tmp/port0";
    if (mVMap.count("ushell")) {
        ushellDevicePath = mVMap["ushell"].as<std::string>();
    }

    auto *socat = Socat::create(ushellDevicePath);

    // initialize ushell command interceptor
    std::string ushellRoot = "/ushell";
    if (mVMap.count("root")) {
        ushellRoot = mVMap["root"].as<std::string>();
    }

    std::string ushellHostMountPoint = "./fs0";
    if (mVMap.count("context")) {
        ushellHostMountPoint = mVMap["context"].as<std::string>();
    }

    UShellCmdInterceptor interceptor(ushellRoot, ushellHostMountPoint);

    std::thread reader(socatReader, socat);

    // Define a name (String)
    std::string userInput;

    do {
        std::cout << "UShell> ";
        std::getline(std::cin, userInput);
        if (std::cin.eof()) {
            break;
        }

        auto interceptResult = interceptor.intercept(userInput);

        if (interceptResult.code == 0 && !interceptResult.handled) {
            socat->write(userInput);
        } else {
            continue;
        }

        std::string socatOutput;
        socat->read(socatOutput);
        std::cout << "UShell> " << socatOutput;
    } while (userInput != "exit");

    running = false;
    reader.join();

    return 0;

    //TODO integrate with real socat
}
