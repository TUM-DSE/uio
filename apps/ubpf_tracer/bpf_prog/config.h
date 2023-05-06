#ifndef CONFIG_H
#define CONFIG_H

#include "bpf_helpers.h"

#include <stdint.h>
#define COUNT_KEY 1252

struct UbpfTracerCtx {
	uint64_t traced_function_address;
};

#endif /* CONFIG_H */
