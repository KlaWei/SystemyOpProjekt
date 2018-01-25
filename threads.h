#ifndef THREADS_H
#define THREADS_H

void libinit();

int thread_create(void *(*func)(void *), void *arg);

void thread_join(int thread_tid, void **retval);

void thread_exit(void *retval);

void thread_yield();

int thread_lock_init(int *m, int initial_val);

void thread_lock(int*);

void thread_unlock(int*);

#endif
    