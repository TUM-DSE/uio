extern void ushell_puts(char *);

// put the string in the .rodata section
char str[] = "hello ushell!\n";

// NOTE: gcc/clang put string literals in .rodata.str.1 section in the
// following case:
// ushell_puts("hello ushell!\n");

__attribute__((section(".text"))) int main()
{
	ushell_puts(str);
	return 0;
}
