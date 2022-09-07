#include <stdio.h>

#include <uk/store.h>

static int my_getter(void *cookie __unused, __u8 *dst)
{
	UK_ASSERT(dst);
	*dst = 42;
	return 0;
}

UK_STORE_STATIC_ENTRY(my_entry, u8, my_getter, NULL, NULL);

int main()
{
	__u8 val;
	static const struct uk_store_entry *my_entry;
	my_entry = uk_store_get_entry(appukstore, NULL, "my_entry");
	uk_store_get_value(my_entry, u8, &val);
	printf("val: %d\n", val);
	return 0;
}
