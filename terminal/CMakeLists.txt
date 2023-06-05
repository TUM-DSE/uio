cmake_minimum_required(VERSION 3.22)
project(ushell_terminal)

set(CMAKE_CXX_STANDARD 17)

FIND_PACKAGE(Boost 1.40 COMPONENTS program_options REQUIRED)

add_subdirectory(ebpf-verifier)
add_subdirectory(uk_bpf_helper)

include_directories(.)
include_directories(./ebpf-verifier/src)
include_directories(./uk_bpf_helper)
include_directories(${Boost_INCLUDE_DIR})

add_executable(ushell_terminal UshellTerminal.cpp device/UShellConsoleDeviceFactory.cpp device/UShellConsoleDeviceFactory.h device/UShellConsoleDevice.cpp device/UShellConsoleDevice.h device/utils/BlockingQueue.h UshellCmdInterceptor.cpp UShellCmdInterceptor.h EBPFVerifier.cpp EBPFVerifier.h parameters.h device/mock/UShellConsoleMock.cpp device/mock/UShellConsoleMock.h)

target_link_libraries(ushell_terminal ${Boost_LIBRARIES} ebpfverifier)