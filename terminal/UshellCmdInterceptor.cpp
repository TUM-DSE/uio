//
// Created by ken on 22.05.23.
//

#include <utility>
#include <vector>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <iostream>
#include <boost/algorithm/string/trim.hpp>

#include "UShellCmdInterceptor.h"
#include "config.hpp"
#include "ebpf_verifier.hpp"

#include "main/utils.hpp"
#include "EBPFVerifier.h"

#include <filesystem>

//TODO: optimization: cache the result of the last successful verification, e.g., via hash of the file
UShellCmdInterceptor::UShellCmdInterceptor(std::string ushellRoot, std::string ushellHostMountPoint) :
        ushellRoot(std::move(ushellRoot)),
        ushellHostMountPoint(std::move(ushellHostMountPoint)) {
}

InterceptionResult UShellCmdInterceptor::intercept(const std::string &in) {
    std::vector<std::string> tokens;
    boost::split(tokens, in, boost::is_any_of("\t "));

    if (!tokens.empty()) {
        std::string command = tokens[0];
        boost::trim(command);

        if (command == "cwd") {
            std::cout << std::filesystem::current_path() << std::endl;
            return {0, true};
        } else if (command == "bpf_exec") {
            if(tokens.size() < 2) {
               std::cout << "PREVAIL> ERROR: Invalid usage, expected: bpf_exec <bpf_file> (<args> <number of args>)" << std::endl;
               return {400, true};
            }

            std::filesystem::path bpfFile(tokens[1]);

            std::cout << "PREVAIL> Security checking bpf file: " << bpfFile.string() << std::endl;
            if (bpfFile.is_absolute()) {
                if (bpfFile.string().find(ushellRoot) != 0) {
                    std::cout << "ERROR: Cannot load an bpf file out of UShell context directory!" << std::endl;
                    return {403, true};
                }

                bpfFile = std::filesystem::relative(bpfFile, ushellRoot);
            }

            auto bpfFileHost = ushellHostMountPoint / bpfFile;
            std::cout << "PREVAIL> Loading bpf file from host: " << bpfFileHost << std::endl;

            // check if the file exists
            if (!std::filesystem::exists(bpfFileHost)) {
                std::cout << "PREVAIL> ERROR: Cannot find bpf file on host!" << std::endl;
                return {404, true};
            }

            // initialize verifier
            EBPFVerifier verifier(ushellHostMountPoint / "symbols.txt", ushellHostMountPoint / "helpers.txt",
                                  ebpf_verifier_default_options);

            // verify bpf file
            bool allOK = true;
            try {
                auto bpfFileSections = verifier.getSections(bpfFileHost);
                std::cout << "PREVAIL> Verifying " << bpfFileSections.size() << " sections..." << std::endl;

                int successCount = 0;
                for(const auto& section : bpfFileSections) {

                    std::cout << "Verifying section: " << section;
                    auto verifyResult = verifier.verify(bpfFileHost, section);
                    std::cout << " (took: " << verifyResult.took << " seconds)" << std::endl;

                    if(verifyResult.ok) {
                        successCount++;
                    } else {
                        allOK = false;
                        std::cout << "ERROR: Verification failed for section: " << section << std::endl;
                    }
                }

                std::cout << "OK: " << successCount << "; Failed: " << bpfFileSections.size() - successCount << std::endl;

                if(successCount != bpfFileSections.size()) {
                    return {406, true};
                }

            } catch (std::runtime_error &e) {
                std::cout << "PREVAIL> ERROR! Security check failed with error: " << e.what() << std::endl;
                return {500, true};
            }

            if (!allOK) {
                std::cout << "PREVAIL> ERROR! Security check failed!" << std::endl;
                return {-1, true};
            }
        }
    }

    return {0, false};
}