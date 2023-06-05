//
// Created by ken on 22.05.23.
//

#ifndef USHELL_TERMINAL_EBPFVERIFIER_HPP
#define USHELL_TERMINAL_EBPFVERIFIER_HPP

#include <string>
#include <vector>
#include <filesystem>
#include "config.hpp"
#include "platform.hpp"

struct eBPFVerifyResult {
    bool ok;
    double took;

};

class EBPFVerifier {
public:
    explicit EBPFVerifier(ebpf_verifier_options_t verifierOptions);

    std::vector<std::string> getSections(const std::filesystem::path &bpfFile);

    struct eBPFVerifyResult verify(const std::filesystem::path &bpfFile, const std::string &desiredSection);

private:
    const ebpf_verifier_options_t mVerifierOptions;

    const ebpf_platform_t *mPlatform;
};


#endif // USHELL_TERMINAL_EBPFVERIFIER_HPP
