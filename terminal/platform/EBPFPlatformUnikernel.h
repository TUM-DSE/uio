#ifndef USHELL_TERMINAL_EBPFPLATFORMUNIKERNEL_H
#define USHELL_TERMINAL_EBPFPLATFORMUNIKERNEL_H


#include <prog_type_list.h>
#include <uk_program_types.h>
#include <platform.hpp>

class EBPFPlatformUnikernel {
public:
    static void init(const HelperFunctionList *helperFunctionList, const BpfProgTypeList *progTypes);

    static const ebpf_platform_t *getPlatform();

private:
    static EbpfProgramType getProgramType(const std::string &section, const std::string &path);

    static EbpfHelperPrototype getHelperPrototype(int32_t n);

    static bool isHelperUsable(int32_t n);

    constexpr const static ebpf_platform_t mPlatform = {
        .get_program_type = EBPFPlatformUnikernel::getProgramType,
        .get_helper_prototype = EBPFPlatformUnikernel::getHelperPrototype,
        .is_helper_usable = EBPFPlatformUnikernel::isHelperUsable,
    };
};


#endif //USHELL_TERMINAL_EBPFPLATFORMUNIKERNEL_H
