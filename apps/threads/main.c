#include <stdio.h>

#include <uk/schedcoop.h>

struct arg {
	int id;
	int n;
};

void thread(void *arg)
{
	struct arg *a = arg;
	int i = 0;
	for (i = 0; i < a->n; i++) {
		printf("[Thread %d] %d\n", a->id, i);
		uk_sched_yield();
		// ukschedcoop is not a preemptive scheduler
		// This will stop the program
		// while(1);
	}
	return;
}

int main()
{
	struct uk_thread *th1, *th2;
	struct arg arg1 = {.id = 1, .n = 10};
	struct arg arg2 = {.id = 2, .n = 10};

	th1 = uk_thread_create("thread1", thread, &arg1);
	th2 = uk_thread_create("thread1", thread, &arg2);
	if (!th1 || !th2) {
		printf("Failed to create a thread\n");
		return 1;
	}

	uk_thread_wait(th1);
	uk_thread_wait(th2);
	printf("exiting...\n");

	return 0;
}
