#include "bpf_helpers.h"
#include "config.h"

int bpf_prog(void *arg)
{
	return (int)arg/0;
}
