#include "EBPFVerifier.hpp"
#include "ebpf_verifier.hpp"
#include "main/utils.hpp"
#include "main/memsize_linux.hpp"
#include "platform/EBPFPlatformUnikernel.h"

EBPFVerifier::EBPFVerifier(const ebpf_verifier_options_t verifierOptions, const HelperFunctionList *helperFunctionList,
                           const BpfProgTypeList *progTypes)
        : mVerifierOptions(verifierOptions) {

    EBPFPlatformUnikernel::init(helperFunctionList, progTypes);
    mPlatform = EBPFPlatformUnikernel::getPlatform();
}

std::vector<std::string>
EBPFVerifier::getSections(const std::filesystem::path &bpfFile) {
    std::vector<raw_program> rawPrograms =
            read_elf(bpfFile, std::string(), &mVerifierOptions, mPlatform);

    std::vector<std::string> sections;
    for (const auto &rawProgram: rawPrograms) {
        sections.push_back(rawProgram.section);
    }

    return sections;
}

struct eBPFVerifyResult
EBPFVerifier::verify(const std::filesystem::path &bpfFile,
                     const std::string &desiredSection) {

    // Read a set of raw program sections from an ELF file.
    std::vector<raw_program> rawPrograms =
            read_elf(bpfFile, desiredSection, &mVerifierOptions, mPlatform);

    // Select the last program section.
    raw_program raw_prog = rawPrograms.back();

    // Convert the raw program section to a set of instructions.
    std::variant<InstructionSeq, std::string> prog_or_error =
            unmarshal(raw_prog);
    if (std::holds_alternative<std::string>(prog_or_error)) {
        throw std::runtime_error(
                "unmarshall error at "
                + std::get<std::string>(prog_or_error));
    }

    auto &prog = std::get<InstructionSeq>(prog_or_error);

    // verify with domain: zoneCrab
    ebpf_verifier_stats_t verifier_stats{};

    clock_t begin = clock();
    const auto result = ebpf_verify_program(std::cout, prog, raw_prog.info,
                                   &mVerifierOptions, &verifier_stats);
    clock_t end = clock();
    
    double elapsed_ns = (double(end) * 1e9  - double(begin) * 1e9) / CLOCKS_PER_SEC;

    if (mVerifierOptions.check_termination
        && (mVerifierOptions.print_failures
            || mVerifierOptions.print_invariants)) {
        std::cout << "Program terminates within "
                  << verifier_stats.max_instruction_count
                  << " instructions\n";
    }

    return {.ok = result, .took = elapsed_ns};
}
