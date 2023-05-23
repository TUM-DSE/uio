/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Costin Lupu <costin.lupu@cs.pub.ro>
 *
 * Copyright (c) 2019, University Politehnica of Bucharest. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/* Based on https://github.com/unikraft/lib-redis/blob/staging/main.c
 */

#include <uk/config.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#if defined(CONFIG_LIBREDIS_MAIN_FUNCTION)
#error Disable "Provide main function" in redis config!
#endif

#define CONFIG_LIBREDIS_SERVER_MAIN_FUNCTION 1

#ifdef CONFIG_LIBUKALLOC_IFSTATS
#include "../common/include/memstat.h"
#endif

struct arg_wrapper {
	int argc;
	char **argv;
};

/*
 * Server thread
 */
#if CONFIG_LIBREDIS_SERVER_MAIN_FUNCTION
extern int redis_server_main(int argc, char *argv[]);

void *server_thread_fn(void *arg)
{
	long rc;
	struct arg_wrapper *aw = arg;

	rc = redis_server_main(aw->argc, aw->argv);
	return (void *) rc;
}
#endif

/*
 * Client thread
 */
#if CONFIG_LIBREDIS_CLIENT_MAIN_FUNCTION
extern int redis_client_main(int argc, char *argv[]);

void *client_thread_fn(void *arg)
{
	long rc;
	struct arg_wrapper *aw = arg;

	/* Wait for environment to be ready */
	sleep(2);

	rc = redis_client_main(aw->argc, aw->argv);
	return (void *) rc;
}
#endif

int main(int argc, char *argv[])
{
#ifdef CONFIG_LIBUKALLOC_IFSTATS
	get_ukalloc_stat();
	while(1);
#endif

	int rc, _rc;
	struct arg_wrapper aw;
#if CONFIG_LIBREDIS_SERVER_MAIN_FUNCTION
	pthread_t thread_server;
#endif
#if CONFIG_LIBREDIS_CLIENT_MAIN_FUNCTION
	pthread_t thread_client;
#endif

	aw.argc = argc;
	aw.argv = argv;

	/* Start server */
#if CONFIG_LIBREDIS_SERVER_MAIN_FUNCTION
	rc = pthread_create(&thread_server, NULL, server_thread_fn, &aw);
	if (rc)
		goto out;
#endif

	/* Start client and wait for it to end */
#if CONFIG_LIBREDIS_CLIENT_MAIN_FUNCTION
	rc = pthread_create(&thread_client, NULL, client_thread_fn, &aw);
	if (rc == 0)
		rc = pthread_join(thread_client, NULL);
#endif

	/* Wait for server to end */
#if CONFIG_LIBREDIS_SERVER_MAIN_FUNCTION
	_rc = pthread_join(thread_server, NULL);
	if (_rc && !rc)
		rc = _rc;
out:
#endif

	return rc;
}
