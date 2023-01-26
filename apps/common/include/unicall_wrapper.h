#ifndef UNICALL_WRAPPER_H
#define UNICALL_WRAPPER_H

#ifdef HAS_MPK
#define unikraft_call_wrapper(fname, ...)			\
do {								\
	        ushell_enable_write();					\
	        fname(__VA_ARGS__);				\
	        ushell_disable_write();				\
} while (0)

#define unikraft_call_wrapper_ret(retval, fname, ...)		\
do {								\
	        ushell_enable_write();					\
	        retval = fname(__VA_ARGS__);			\
	        ushell_disable_write();				\
} while (0)

#define unikraft_write_var(var, values)		\
do {								\
	        ushell_enable_write();					\
	        var = values;			\
	        ushell_disable_write();				\
} while (0)

extern int ushell_enable_write();
extern int ushell_disable_write();

#else /* HAS_MPK */

#define unikraft_call_wrapper(fname, ...)			\
do {								\
	        fname(__VA_ARGS__);				\
} while (0)

#define unikraft_call_wrapper_ret(retval, fname, ...)		\
do {								\
	        retval = fname(__VA_ARGS__);			\
} while (0)

#define unikraft_write_var(var, values)		\
do {								\
	        var = values;			\
} while (0)

#endif /* HAS_MPK */
#endif /* MPK_WRAPPER_H */
