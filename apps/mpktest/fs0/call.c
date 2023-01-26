
extern unsigned int sleep(unsigned);

__attribute__((section(".text"))) int main()
{
	sleep(1);
	return 0;
}
