
int bpf_prog(void *arg)
{
	int *p = arg;
	return *(p+1000);
}
