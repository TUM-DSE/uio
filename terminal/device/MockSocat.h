//
// Created by ken on 22.05.23.
//

#ifndef USHELL_TERMINAL_MOCKSOCAT_H
#define USHELL_TERMINAL_MOCKSOCAT_H

#include <deque>
#include "Socat.h"
#include "device/utils/BlockingQueue.h"

class MockSocat : public Socat {
public:
    explicit MockSocat(std::string path);

    unsigned long read(std::string &out) override;
    unsigned long write(const std::string &in) override;

    int open() override;

    ~MockSocat();

private:
    const std::string path;
    BlockingQueue<std::string> mBuffer;
};


#endif //USHELL_TERMINAL_MOCKSOCAT_H
