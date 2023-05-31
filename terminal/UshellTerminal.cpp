//
// Created by Ken on 2023/5/22.
//

#include <iostream>
#include <boost/program_options.hpp>
#include <thread>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/trim.hpp>

#include <csignal>

#include "device/UShellConsoleDeviceFactory.h"
#include "UShellCmdInterceptor.h"
#include "parameters.h"

void systemSignalHandler(int signal)
{
	exit(signal);
}

void readUntilPrompt(UShellConsoleDevice *device, bool printPrompt = true)
{
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

int main(int argc, char **argv)
{
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
	uShellConsoleDevice->write("");
	readUntilPrompt(uShellConsoleDevice, false);

	if (!uShellConsoleDevice->write(MOUNT_INFO_COMMAND)) {
		std::cerr << "ERROR Failed to write ushell console"
			  << std::endl;
		return -1;
	}

	std::string ushellMountInfo;
	if (!uShellConsoleDevice->readline(ushellMountInfo)) {
		std::cerr << "ERROR Failed to read ushell mount-info"
			  << std::endl;
		return -1;
	}

	const char *mountInfoResponsePrefix = MOUNT_INFO_RESPONSE_PREFIX "=";
	if (ushellMountInfo.find(mountInfoResponsePrefix) != 0) {
		std::cerr << "ERROR Invalid ushell mount-info response: "
			  << ushellMountInfo << std::endl;
		return -1;
	}

	std::string ushellMountPath =
	    ushellMountInfo.substr(strlen(mountInfoResponsePrefix));
	boost::trim_right(ushellMountPath);

	std::vector<std::string> tokens;
	boost::split(tokens, ushellMountPath, boost::is_any_of(":"));
	if (tokens.size() != 2) {
		std::cerr << "ERROR Invalid ushell mount-info response: "
			  << ushellMountInfo << std::endl;
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

	UShellCmdInterceptor interceptor(ushellRoot, ushellHostMountPoint);

	// initialize signal handler
	struct sigaction sigIntHandler {
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

		if (interceptResult.code == 0 && !interceptResult.handled) {
			auto bytesWritten =
			    uShellConsoleDevice->write(userInput);
			// we can write more bytes than the user actual input
			// e.g., getline() removes the trailing '\n'
			// but we need it to trigger the command execution
			if (bytesWritten < userInput.size()) {
				throw std::runtime_error(
				    "FATAL Failed to write to USHell "
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
