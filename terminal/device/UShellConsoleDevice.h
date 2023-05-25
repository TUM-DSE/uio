#ifndef USHELL_TERMINAL_USHELLCONSOLEDEVICE_H
#define USHELL_TERMINAL_USHELLCONSOLEDEVICE_H

class UShellConsoleDevice {
public:
    explicit UShellConsoleDevice(const std::string& path);

    unsigned long read(std::string &out) const;
    unsigned long write(const std::string &in) const;

    ~UShellConsoleDevice();

private:
    int socketFd;
};


#endif // USHELL_TERMINAL_USHELLCONSOLEDEVICE_H
