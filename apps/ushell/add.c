inline int atoi(char *str)
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

int add(int argc, char *argv[])
{
	int a, b;
	a = b = 0;
	if (argc >= 2)
		a = atoi(argv[1]);
	if (argc >= 3)
		b = atoi(argv[2]);
	return a + b;
}

