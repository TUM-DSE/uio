#ifndef USHELL_TERMINAL_USHELLCONSOLEDEVICEFACTORY_HPP
#define USHELL_TERMINAL_USHELLCONSOLEDEVICEFACTORY_HPP

#include <string>
#include "UShellConsoleDevice.hpp"

UShellConsoleDevice *createUshellConsoleDevice(const std::string &path);

#endif // USHELL_TERMINAL_USHELLCONSOLEDEVICEFACTORY_HPP
