#include <unistd.h>
#include <iostream>

#include "UShellConsoleDeviceFactory.hpp"
#include "UShellConsoleDevice.hpp"
#include "device/mock/UShellConsoleMock.hpp"

//#define MOCK_SOCAT

UShellConsoleDevice *createUshellConsoleDevice(const std::string &path)
{
#ifdef MOCK_SOCAT
	pid_t pid = fork();
	if (pid < 0) {
		throw std::runtime_error(
		    "Failed to initialize mocked ushell console server");
	} else if (pid == 0) {
		UShellConsoleMock::start(path);
	} else {
		sleep(1);
		const UShellConsoleDevice *uShellConsoleDevice =
		    new UShellConsoleDevice(path);
		return (UShellConsoleDevice *)uShellConsoleDevice;
	}

#else
	const UShellConsoleDevice *uShellConsoleDevice =
	    new UShellConsoleDevice(path);
	return (UShellConsoleDevice *)uShellConsoleDevice;
#endif
}
