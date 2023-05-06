#ifndef UNICALL_WRAPPER_H
#define UNICALL_WRAPPER_H

#ifdef HAS_MPK

int ushell_disable_write();
int ushell_enable_write();
int ushell_write_is_enabled();
#define unikraft_call_wrapper(fname, ...)                                      \
	do {                                                                   \
		if (ushell_write_is_enabled()) {                               \
			fname(__VA_ARGS__);                                    \
		} else {                                                       \
			ushell_enable_write();                                 \
			fname(__VA_ARGS__);                                    \
			ushell_disable_write();                                \
		}                                                              \
	} while (0)

#define unikraft_call_wrapper_ret(retval, fname, ...)                          \
	do {                                                                   \
		if (ushell_write_is_enabled()) {                               \
			retval = fname(__VA_ARGS__);                           \
		} else {                                                       \
			ushell_enable_write();                                 \
			retval = fname(__VA_ARGS__);                           \
			ushell_disable_write();                                \
		}                                                              \
	} while (0)

#define unikraft_write_var(var, values)                                        \
	do {                                                                   \
		if (ushell_write_is_enabled()) {                               \
			var = values;                                          \
		} else {                                                       \
			ushell_enable_write();                                 \
			var = values;                                          \
			ushell_disable_write();                                \
		}                                                              \
	} while (0)

#else

#define unikraft_call_wrapper(fname, ...)			\
do {								\
	fname(__VA_ARGS__);					\
} while (0)

#define unikraft_call_wrapper_ret(retval, fname, ...)		\
do {								\
	retval = fname(__VA_ARGS__);				\
} while (0)

#define unikraft_write_var(var, values)		\
do {								\
			var = values;			\
} while (0)

#endif /* HAS_MPK */

#endif /* UNICALL_WRAPPER_H */
