//
// Created by ken on 22.05.23.
//

#ifndef USHELL_TERMINAL_USHELLCMDINTERCEPTOR_H
#define USHELL_TERMINAL_USHELLCMDINTERCEPTOR_H

#include <string>
#include <filesystem>


struct InterceptionResult {
    int code;
    bool handled;
};

class UShellCmdInterceptor {
public:
    UShellCmdInterceptor(std::string ushellRoot, std::string ushellHostMountPoint);

    InterceptionResult intercept(const std::string& in);

private:
    const std::filesystem::path ushellRoot;
    const std::filesystem::path ushellHostMountPoint;
};


#endif //USHELL_TERMINAL_USHELLCMDINTERCEPTOR_H
