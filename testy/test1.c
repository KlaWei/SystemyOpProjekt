#include <stdio.h>
#include "threads.h"

void* hello(void *arg) {
    printf("%s\n", (char*)arg);
    thread_exit(NULL);
}


int main() {
    const char* msg1 = "Child thread 1";
    const char* msg2 = "Child thread 2";
    const char* msg3 = "Child thread 3";
    libinit();
    
    int th1, th2,th3;
    th1 = thread_create(hello, (void*)msg1);
    th2 = thread_create(hello, (void*)msg2);
    th3 = thread_create(hello, (void*)msg3);

    thread_join(th1, NULL);
    thread_join(th2, NULL);
    thread_join(th3, NULL);

    printf("Parent thread\n");

    thread_exit(NULL);
    printf("hahah\n");
}