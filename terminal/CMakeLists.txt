cmake_minimum_required(VERSION 3.22)
project(ushell_terminal)

set(CMAKE_CXX_STANDARD 17)

FIND_PACKAGE(Boost 1.40 COMPONENTS program_options REQUIRED)

add_subdirectory(ebpf-verifier)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../libs/uk_bpf_helper uk_bpf_helper)

include_directories(.)
include_directories(./ebpf-verifier/src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../libs/uk_bpf_helper)
include_directories(${Boost_INCLUDE_DIR})

OPTION(CHECK_TERMINATION "Whether enable BPF verifier feature checking program termination" OFF)

IF(CHECK_TERMINATION)
    ADD_DEFINITIONS(-DCHECK_TERMINATION)
ENDIF(CHECK_TERMINATION)

add_executable(ushell_terminal UshellTerminal.cpp
        device/UShellConsoleDeviceFactory.cpp device/UShellConsoleDeviceFactory.hpp
        device/UShellConsoleDevice.cpp device/UShellConsoleDevice.hpp device/utils/BlockingQueue.h
        UshellCmdInterceptor.cpp UShellCmdInterceptor.hpp
        EBPFVerifier.cpp EBPFVerifier.hpp parameters.h
        device/mock/UShellConsoleMock.cpp device/mock/UShellConsoleMock.hpp
        platform/EBPFPlatformUnikernel.cpp platform/EBPFPlatformUnikernel.h)

target_link_libraries(ushell_terminal ${Boost_LIBRARIES} ebpfverifier uk_bpf_helper)