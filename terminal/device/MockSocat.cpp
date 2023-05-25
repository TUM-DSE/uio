//
// Created by ken on 22.05.23.
//

#include <mutex>
#include <utility>
#include <iostream>
#include "MockSocat.h"


MockSocat::MockSocat(std::string path) : path(std::move(path)) {
}

unsigned long MockSocat::read(std::string &out) {
    mBuffer.waitAndPop(out);

    std::cerr<< "DEBUG Read: " << out;

    return out.size();
}

unsigned long MockSocat::write(const std::string &in) {
    std::cerr<< "DEBUG Write: " << in << std::endl;

    mBuffer.push(std::string("Mocked response from command: " + in + "\n"));

    return in.size();
}

int MockSocat::open() {
    // do nothing, we open nothing in the mock
    std::cerr << "DEBUG MockSocat::open() called" << std::endl;
    return 0;
}

MockSocat::~MockSocat() {
    std::cerr << "DEBUG MockSocat::~MockSocat() called" << std::endl;
}
