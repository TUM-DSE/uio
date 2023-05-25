//
// Created by ken on 22.05.23.
//

#include <stdexcept>
#include "Socat.h"
#include "MockSocat.h"

#define MOCK_SOCAT

Socat *Socat::create(const std::string &path) {
#ifdef MOCK_SOCAT
    const MockSocat* socat = new MockSocat(path);
    return (Socat *) socat;
#else
    throw std::runtime_error("Not implemented");
    return nullptr;
#endif
}
