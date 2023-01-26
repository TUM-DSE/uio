extern int ushell_mpk_test_var;

__attribute__((section(".text"))) int main()
{
	ushell_mpk_test_var = 42;
	return 0;
}
