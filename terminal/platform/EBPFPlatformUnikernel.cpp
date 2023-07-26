#include "EBPFPlatformUnikernel.h"

#include <unordered_map>

static const ebpf_context_descriptor_t ukBPFContextUnspecified = {0, -1, -1, -1};
static std::vector<EbpfProgramType> ukProgramTypes;
static std::map<uint32_t, EbpfHelperPrototype> ukPrototypes;

const ebpf_platform_t *EBPFPlatformUnikernel::getPlatform() {
    return &EBPFPlatformUnikernel::mPlatform;
}

EbpfProgramType EBPFPlatformUnikernel::getProgramType(const std::string &section, const std::string &path) {
    EbpfProgramType type{};

    for (const EbpfProgramType &programType: ukProgramTypes) {
        for (const std::string &prefix: programType.section_prefixes) {
            if (section.find(prefix) == 0) {
                return programType;
            }
        }
    }

    return ukProgramTypes[0]; // 0 for unspecified program type.
}

EbpfHelperPrototype EBPFPlatformUnikernel::getHelperPrototype(int32_t n) {
    if (!EBPFPlatformUnikernel::isHelperUsable(n))
        throw std::exception();
    return ukPrototypes[n];
}

bool EBPFPlatformUnikernel::isHelperUsable(int32_t n) {
    if (n >= ukPrototypes.size() || n < 0) {
        return false;
    }

    // If the helper has a context_descriptor, it must match the hook's context_descriptor.
    if ((ukPrototypes[n].context_descriptor != nullptr) &&
        (ukPrototypes[n].context_descriptor != global_program_info->type.context_descriptor)) {
        return false;
    }

    return true;
}

void EBPFPlatformUnikernel::init(const HelperFunctionList *helperFunctionList, const BpfProgTypeList *progTypes) {
    ukProgramTypes.clear();
    ukProgramTypes.resize(1 + progTypes->m_length);

    ukProgramTypes[0].name = "unspecified";
    ukProgramTypes[0].context_descriptor = &ukBPFContextUnspecified;
    ukProgramTypes[0].platform_specific_data = 0;
    ukProgramTypes[0].section_prefixes = {"unspecified"};
    ukProgramTypes[0].is_privileged = false;

    std::unordered_map<uint64_t, ebpf_context_descriptor_t *> contextDescriptors;
    contextDescriptors[UK_EBPF_PROG_TYPE_UNSPECIFIED] = nullptr;

    size_t counter = 1;
    for (auto *progType = progTypes->m_head; progType != nullptr; progType = progType->m_next, counter++) {
        // init context descriptors
        auto *descriptor = new ebpf_context_descriptor_t;
        descriptor->size = progType->ctx_descriptor_struct_size;
        descriptor->data = progType->offset_to_data_ptr;
        descriptor->end = progType->offset_to_data_end_ptr;
        descriptor->meta = progType->offset_to_ctx_metadata;

        contextDescriptors[progType->prog_type_id] = descriptor;

        ukProgramTypes[counter].name = progType->m_prog_type_name;
        ukProgramTypes[counter].context_descriptor = descriptor;
        ukProgramTypes[counter].platform_specific_data = progType->prog_type_id;
        ukProgramTypes[counter].section_prefixes = {progType->m_prog_type_name};
        ukProgramTypes[counter].is_privileged = progType->privileged;
    }

    ukPrototypes.clear();
    for (auto *helper = helperFunctionList->m_head; helper != nullptr; helper = helper->m_next) {
        ukPrototypes[helper->m_index].name = helper->m_function_signature.m_function_name;
        ukPrototypes[helper->m_index].return_type = static_cast<ebpf_return_type_t>(helper->m_function_signature.m_return_type);
        ukPrototypes[helper->m_index].reallocate_packet = false;// TODO
        ukPrototypes[helper->m_index].context_descriptor = contextDescriptors[helper->m_prog_type_id];

        for (int index = 0; index < sizeof(ukPrototypes[helper->m_index].argument_type) /
                                    sizeof(ukPrototypes[helper->m_index].argument_type[0]); index++) {
            ukPrototypes[helper->m_index].argument_type[index] = static_cast<ebpf_argument_type_t>(helper->m_function_signature.m_arg_types[index]);
        }
    }
}