#include <iostream>
#include <boost/program_options.hpp>
#include <thread>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/trim.hpp>

#include <csignal>

#include "device/UShellConsoleDeviceFactory.hpp"
#include "UShellCmdInterceptor.hpp"
#include "parameters.h"
#include "memsize_linux.hpp"

extern "C" {
#include "uk_bpf_helper_utils.h"
}

// make sure that the uk bpf platform definitions are compatible with the PREVAIL definitions
static_assert(EBPF_RETURN_TYPE_INTEGER == UK_EBPF_RETURN_TYPE_INTEGER);
static_assert(EBPF_RETURN_TYPE_PTR_TO_MAP_VALUE_OR_NULL == UK_EBPF_RETURN_TYPE_PTR_TO_MAP_VALUE_OR_NULL);
static_assert(EBPF_RETURN_TYPE_INTEGER_OR_NO_RETURN_IF_SUCCEED == UK_EBPF_RETURN_TYPE_INTEGER_OR_NO_RETURN_IF_SUCCEED);
static_assert(EBPF_RETURN_TYPE_UNSUPPORTED == UK_EBPF_RETURN_TYPE_UNSUPPORTED);

static_assert(EBPF_ARGUMENT_TYPE_DONTCARE == UK_EBPF_ARGUMENT_TYPE_DONTCARE);
static_assert(EBPF_ARGUMENT_TYPE_ANYTHING == UK_EBPF_ARGUMENT_TYPE_ANYTHING);
static_assert(EBPF_ARGUMENT_TYPE_CONST_SIZE == UK_EBPF_ARGUMENT_TYPE_CONST_SIZE);
static_assert(EBPF_ARGUMENT_TYPE_CONST_SIZE_OR_ZERO == UK_EBPF_ARGUMENT_TYPE_CONST_SIZE_OR_ZERO);
static_assert(EBPF_ARGUMENT_TYPE_PTR_TO_CTX == UK_EBPF_ARGUMENT_TYPE_PTR_TO_CTX);
static_assert(EBPF_ARGUMENT_TYPE_PTR_TO_MAP == UK_EBPF_ARGUMENT_TYPE_PTR_TO_MAP);
static_assert(EBPF_ARGUMENT_TYPE_PTR_TO_MAP_OF_PROGRAMS == UK_EBPF_ARGUMENT_TYPE_PTR_TO_MAP_OF_PROGRAMS);
static_assert(EBPF_ARGUMENT_TYPE_PTR_TO_MAP_KEY == UK_EBPF_ARGUMENT_TYPE_PTR_TO_MAP_KEY);
static_assert(EBPF_ARGUMENT_TYPE_PTR_TO_MAP_VALUE == UK_EBPF_ARGUMENT_TYPE_PTR_TO_MAP_VALUE);
static_assert(EBPF_ARGUMENT_TYPE_PTR_TO_READABLE_MEM == UK_EBPF_ARGUMENT_TYPE_PTR_TO_READABLE_MEM);
static_assert(EBPF_ARGUMENT_TYPE_PTR_TO_READABLE_MEM_OR_NULL == UK_EBPF_ARGUMENT_TYPE_PTR_TO_READABLE_MEM_OR_NULL);
static_assert(EBPF_ARGUMENT_TYPE_PTR_TO_WRITABLE_MEM == UK_EBPF_ARGUMENT_TYPE_PTR_TO_WRITABLE_MEM);
static_assert(EBPF_ARGUMENT_TYPE_UNSUPPORTED == UK_EBPF_ARGUMENT_TYPE_UNSUPPORTED);

void systemSignalHandler(int signal) {
    exit(signal);
}

void readUntilPrompt(UShellConsoleDevice *device, bool printPrompt = true) {
    char lastChar = 0;
    while (true) {
        char c = device->read();
        if (c < 0) {
            throw std::runtime_error(
                    "Could not read from ushell console: IO error");
        }

        if (lastChar == '>') {
            if (c == ' ') {
                if (printPrompt) {
                    std::cout << USHELL_TERMINAL_PROMPT;
                }
                break;
            } else {
                std::cout << lastChar;
            }
        }

        if (c != '>') {
            std::cout << c << std::flush;
        }

        lastChar = c;
    }
}

int main(int argc, char **argv) {
    // setup program options description
    boost::program_options::options_description ushellTerminalOptions(
            "UShell Terminal Options");
    ushellTerminalOptions.add_options()("help,h", "Produce help message")(
            "ushell,u", boost::program_options::value<std::string>(),
            "The virtio device UShell lives on (default: /tmp/port0)");

    boost::program_options::variables_map mVMap;

    try {
        // parse program options

        boost::program_options::store(
                boost::program_options::parse_command_line(
                        argc, argv, ushellTerminalOptions),
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

    // create uShellConsoleDevice device
    std::string ushellDevicePath = "/tmp/port0";
    if (mVMap.count("ushell")) {
        ushellDevicePath = mVMap["ushell"].as<std::string>();
    }

    auto *uShellConsoleDevice = createUshellConsoleDevice(ushellDevicePath);
    if (!uShellConsoleDevice->write("")) {
        std::cerr
                << "Fatal Could not connect to ushell console: IO error"
                << std::endl;
        return -1;
    }
    readUntilPrompt(uShellConsoleDevice, false);

    if (!uShellConsoleDevice->write(MOUNT_INFO_COMMAND)) {
        std::cerr << "Fatal Failed to write ushell console"
                  << std::endl;
        return -1;
    }

    std::string ushellMountInfo;
    if (!uShellConsoleDevice->readline(ushellMountInfo)) {
        std::cerr << "Fatal Failed to read ushell mount-info"
                  << std::endl;
        return -1;
    }

    const char *mountInfoResponsePrefix = MOUNT_INFO_RESPONSE_PREFIX "=";
    if (ushellMountInfo.find(mountInfoResponsePrefix) != 0) {
        std::cerr << "Fatal Invalid ushell mount-info response: "
                  << ushellMountInfo << std::endl;
        return -1;
    }

    std::string ushellMountPath =
            ushellMountInfo.substr(strlen(mountInfoResponsePrefix));
    boost::trim_right(ushellMountPath);

    std::vector<std::string> tokens;
    boost::split(tokens, ushellMountPath, boost::is_any_of(":"));
    if (tokens.size() != 2) {
        std::cerr << "Fatal Invalid ushell mount-info response: "
                  << ushellMountInfo << std::endl;
        return -1;
    }

    // read bpf helper function definitions
    readUntilPrompt(uShellConsoleDevice, false);
    if (!uShellConsoleDevice->write(BPF_HELPER_INFO_COMMAND)) {
        std::cerr << "Fatal Failed to write ushell console"
                  << std::endl;
        return -1;
    }

    std::string ushellBpfHelperInfoRaw;
    if (!uShellConsoleDevice->readline(ushellBpfHelperInfoRaw)) {
        std::cerr << "Fatal Failed to read ushell bpf-helper-info"
                  << std::endl;
        return -1;
    }

    const char *helperInfoResponsePrefix =
            BPF_HELPER_FUNCTION_INFO_RESPONSE_PREFIX "=";
    if (ushellBpfHelperInfoRaw.find(helperInfoResponsePrefix) != 0) {
        std::cerr << "Fatal Invalid ushell bpf-helper-info response: "
                  << ushellBpfHelperInfoRaw << std::endl;
        return -1;
    }

    std::string ushellBpfHelperInfo =
            ushellBpfHelperInfoRaw.substr(strlen(helperInfoResponsePrefix));
    boost::trim_right(ushellBpfHelperInfo);

    const auto *ushellBpfHelperList =
            unmarshall_bpf_helper_definitions(ushellBpfHelperInfo.c_str());
    if (ushellBpfHelperList) {
        std::cout << "TERMINAL> Loaded "
                  << ushellBpfHelperList->m_length
                  << " bpf helper function definitions." << std::endl;
    } else {
        std::cerr << "Fatal Failed to load UShell bpf helper function "
                     "definitions."
                  << std::endl;
        return -1;
    }

    // no readUntilPrompt here since prog-type info is provided within the same command as the helper info
    std::string ushellBpfProgTypeInfoRaw;
    if (!uShellConsoleDevice->readline(ushellBpfProgTypeInfoRaw)) {
        std::cerr << "Fatal Failed to read ushell bpf-prog-type-info"
                  << std::endl;
        return -1;
    }

    const char *progTypeInfoResponsePrefix =
            BPF_PROG_TYPE_INFO_RESPONSE_PREFIX "=";
    if (ushellBpfProgTypeInfoRaw.find(progTypeInfoResponsePrefix) != 0) {
        std::cerr << "Fatal Invalid ushell bpf-prog-type-info response: "
                  << ushellBpfProgTypeInfoRaw << std::endl;
        return -1;
    }

    std::string ushellBpfProgTypeInfo =
            ushellBpfProgTypeInfoRaw.substr(strlen(progTypeInfoResponsePrefix));
    boost::trim_right(ushellBpfProgTypeInfo);

    const auto *ushellBpfProgTypeList = unmarshall_bpf_prog_types(ushellBpfProgTypeInfo.c_str());
    if (ushellBpfProgTypeList) {
        std::cout << "TERMINAL> Loaded "
                  << ushellBpfProgTypeList->m_length
                  << " bpf program types." << std::endl;
    } else {
        std::cerr << "Fatal Failed to load UShell bpf program types."
                  << std::endl;
        return -1;
    }

    // initialize ushell command interceptor
    std::string ushellHostMountPoint = tokens.at(0);
    if (mVMap.count("context")) {
        ushellHostMountPoint = mVMap["context"].as<std::string>();
    }

    std::string ushellRoot = tokens.at(1);
    if (mVMap.count("root")) {
        ushellRoot = mVMap["root"].as<std::string>();
    }

    std::cout << "TERMINAL> DEBUG UShell mounted from "
              << ushellHostMountPoint << " to " << ushellRoot << std::endl;

    // Start of the main terminal cycles
    readUntilPrompt(uShellConsoleDevice);

    auto verifierOptions = ebpf_verifier_default_options;
# ifdef CHECK_TERMINATION
    verifierOptions.check_termination = true;
# endif
    // verifierOptions.print_invariants = true;
    verifierOptions.print_failures = true;
    verifierOptions.print_line_info = true;
    verifierOptions.allow_division_by_zero = false;

    UShellCmdInterceptor interceptor(ushellRoot, ushellHostMountPoint,
                                     ushellBpfHelperList, ushellBpfProgTypeList, verifierOptions);

    // initialize signal handler
    struct sigaction sigIntHandler{
    };
    sigIntHandler.sa_handler = systemSignalHandler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
    sigaction(SIGINT, &sigIntHandler, nullptr);

    // Define a name (String)
    std::string userInput;

    do {
        std::getline(std::cin, userInput);
        if (std::cin.eof()) {
            break;
        }

        auto interceptResult = interceptor.intercept(userInput);
        std::cout << "TERMINAL> Max stack size: " << resident_set_size_kb() << " kb" << std::endl;

        if (interceptResult.code == 0 && !interceptResult.handled) {
            auto bytesWritten =
                    uShellConsoleDevice->write(userInput);
            // we can write more bytes than the user actual input
            // e.g., getline() removes the trailing '\n'
            // but we need it to trigger the command execution
            if (bytesWritten < userInput.size()) {
                throw std::runtime_error(
                        "FATAL Failed to write to UShell "
                        "console: Broken pipe");
            }

            readUntilPrompt(uShellConsoleDevice);
        } else {
            std::cout << USHELL_TERMINAL_PROMPT;
            continue;
        }
        
    } while (userInput != "exit");

    return 0;
}
