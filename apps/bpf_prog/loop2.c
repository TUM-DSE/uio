#include "bpf_helpers.h""

int bpf_prog(void *arg)
{
	int a = 0;
	while(1) {
		bpf_time_get_ns();
	}
	return 0;
}
