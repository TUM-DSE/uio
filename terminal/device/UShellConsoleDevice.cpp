#include <mutex>
#include <iostream>
#include <sys/un.h>
#include <sys/socket.h>
#include <unistd.h>
#include "UShellConsoleDevice.h"

UShellConsoleDevice::UShellConsoleDevice(const std::string &path)
{
	// create socket
	sockaddr_un address{};
	address.sun_family = AF_UNIX;
	strcpy(address.sun_path, path.c_str());

	socketFd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (connect(socketFd, (sockaddr *)(&address), sizeof(address)) != 0) {
		std::cerr << errno << std::endl;
		throw std::runtime_error(
		    "Could not connect to ushell console");
	}
}

unsigned long UShellConsoleDevice::read(std::string &out) const
{
	char buffer[256];
	ssize_t bytesRead = ::read(socketFd, buffer, sizeof(buffer));
	if (bytesRead > 0) {
		out += std::string(buffer, bytesRead);
	}

	return out.size();
}

unsigned long UShellConsoleDevice::write(const std::string &in) const
{
	const std::string issuedCommand = in + "\n";
	return ::write(socketFd, issuedCommand.c_str(), issuedCommand.size());
}

UShellConsoleDevice::~UShellConsoleDevice()
{
	close(socketFd);
}
