#ifndef USHELL_TERMINAL_USHELLCONSOLEDEVICEFACTORY_H
#define USHELL_TERMINAL_USHELLCONSOLEDEVICEFACTORY_H

#include <string>
#include "UShellConsoleDevice.h"

UShellConsoleDevice *createUshellConsoleDevice(const std::string &path);

#endif // USHELL_TERMINAL_USHELLCONSOLEDEVICEFACTORY_H
