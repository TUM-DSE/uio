#ifndef UNICALL_WRAPPER_H
#define UNICALL_WRAPPER_H

#ifdef HAS_MPK
#define unikraft_call_wrapper(fname, ...)			\
do {								\
	        enable_write();					\
	        fname(__VA_ARGS__);				\
	        disable_write();				\
} while (0)

#define unikraft_call_wrapper_ret(retval, fname, ...)		\
do {								\
	        enable_write();					\
	        retval = fname(__VA_ARGS__);			\
	        disable_write();				\
} while (0)

extern int enable_write();
extern int disable_write();

#else /* HAS_MPK */

#define unikraft_call_wrapper(fname, ...)			\
do {								\
	        fname(__VA_ARGS__);				\
} while (0)

#define unikraft_call_wrapper_ret(retval, fname, ...)		\
do {								\
	        retval = fname(__VA_ARGS__);			\
} while (0)

#endif /* HAS_MPK */
#endif /* MPK_WRAPPER_H */
