
int atoi(char *str)
{
	int a = 0;
	char *p = str;
	while (*p != '\0' && *p >= '0' && *p <= '9') {
		a *= 10;
		a += (*p - '0');
		p++;
	}
	return a;
}

__attribute__((section(".text"))) int main(int argc, char *argv[])
{
	unsigned long a = 0;
	int i, j, n = 30000;
	if (argc >= 2) {
		n = atoi(argv[1]);
	}
	for (i = 0; i < n; i++) {
		for (j = 0; j < n; j++) {
			a += j;
		}
	}
	return a;
}
