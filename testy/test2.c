#include <stdio.h>
#include "threads.h"

int sem1;

int value = 20;

void* hello(void *arg) {
    printf("%s\n", (char*)arg);
    thread_yield();
    printf("im here\n");
    thread_exit(NULL);

    return 0;
}

void *test_sem(void *arg) {
    thread_lock(&sem1);
    value--;
    thread_yield();
    thread_unlock(&sem1);
    thread_exit(NULL);

    return 0;
}

int main() {
    const char* msg1 = "Child thread 1";
    const char* msg2 = "Child thread 2";
    libinit();
    int th1, th2;
    th1 = thread_create(hello, (void*)msg1);
    th2 = thread_create(hello, (void*)msg2);

    thread_join(th1, NULL);
    thread_join(th2, NULL);

    thread_lock_init(&sem1, 2);

    int th3, th4, th5, th6;
    th3 = thread_create(test_sem, NULL);
    th4 = thread_create(test_sem, NULL);
    th5 = thread_create(test_sem, NULL);
    th6 = thread_create(test_sem, NULL);

    thread_join(th3, NULL);
    thread_join(th4, NULL);
    thread_join(th5, NULL);
    thread_join(th6, NULL);

    printf("value = %d\n", value);


    printf("Parent thread\n");

    thread_exit(NULL);
}