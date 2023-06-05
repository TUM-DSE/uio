#include <mutex>
#include <iostream>
#include <sys/un.h>
#include <sys/socket.h>
#include <unistd.h>
#include "UShellConsoleDevice.hpp"

UShellConsoleDevice::UShellConsoleDevice(const std::string &path)
{
	// create socket
	sockaddr_un address{};
	address.sun_family = AF_UNIX;
	strcpy(address.sun_path, path.c_str());

	socketFd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (connect(socketFd, (sockaddr *)(&address), sizeof(address)) != 0) {
		std::cerr << errno << std::endl;
		throw std::runtime_error("Could not connect to ushell console");
	}
}

char UShellConsoleDevice::read() const
{
	char buffer = 0;
	ssize_t bytesRead = ::read(socketFd, &buffer, sizeof(buffer));

	return bytesRead >= 1 ? buffer : (char)-1;
}

unsigned long UShellConsoleDevice::readline(std::string &out) const
{
	char buffer = 0;
	ssize_t bytesRead = ::read(socketFd, &buffer, sizeof(buffer));
	while (bytesRead > 0 && buffer != '\n') {
		out += std::string(&buffer, bytesRead);
		bytesRead = ::read(socketFd, &buffer, sizeof(buffer));
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
