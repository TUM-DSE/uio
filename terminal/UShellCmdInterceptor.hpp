//
// Created by ken on 22.05.23.
//

#ifndef USHELL_TERMINAL_USHELLCMDINTERCEPTOR_HPP
#define USHELL_TERMINAL_USHELLCMDINTERCEPTOR_HPP

#include <string>
#include <filesystem>
#include "prog_type_list.h"
#include "EBPFVerifier.hpp"

extern "C" {
#include <helper_function_list.h>
}

struct InterceptionResult {
    int code;
    bool handled;
};

class UShellCmdInterceptor {
public:
    UShellCmdInterceptor(std::string ushellRoot,
                         std::string ushellHostMountPoint,
                         const HelperFunctionList *helperFunctionList,
                         const BpfProgTypeList *progTypes,
                         const ebpf_verifier_options_t& verifierOptions);

    InterceptionResult intercept(const std::string &in);

private:
    const std::filesystem::path ushellRoot;
    const std::filesystem::path ushellHostMountPoint;
    EBPFVerifier mVerifier;
};

#endif // USHELL_TERMINAL_USHELLCMDINTERCEPTOR_HPP
