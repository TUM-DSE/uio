//
// Created by ken on 22.05.23.
//

#ifndef USHELL_TERMINAL_SOCAT_H
#define USHELL_TERMINAL_SOCAT_H

#include <string>

class Socat {
public:
    virtual unsigned long read(std::string &out) = 0;

    virtual unsigned long write(const std::string &in) = 0;

    static Socat *create(const std::string &path);

protected:
    virtual int open() = 0;
};


#endif //USHELL_TERMINAL_SOCAT_H
