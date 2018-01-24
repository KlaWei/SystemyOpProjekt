#ifndef THREADS_H
#define THREADS_H

// creating main thread
void libinit();

// creating a thread a putting it on a queue
int thread_create(void *(*start_routine)(void *), void *arg);

// for now assuming that retval is NULL, ! TODO later
void thread_join(int thread_tid, void **retval);

void thread_exit(void *retval);

void thread_yield();

#endif
    