#include <stdio.h>
#include "threads.h"

int arr_sem1;

int arr[5];
int ind=0;

int value = 20;


void *fill_arr(void *arg) {
    thread_lock(&arr_sem1);
    if (ind<5) arr[ind] = (*((int*)arg));
    ind++;
    //thread_yield();
    thread_unlock(&arr_sem1);
    thread_exit(NULL);

    return 0;
}



int main() {
    libinit();
    int th1, th2, th3, th4, th5;

    int arr2[5] = {1,2,3,4,5};

    thread_lock_init(&arr_sem1, 1);
    th1 = thread_create(fill_arr, (void*)&arr2[0]);
    th2 = thread_create(fill_arr, (void*)&arr2[1]);
    th3 = thread_create(fill_arr, (void*)&arr2[2]);
    th4 = thread_create(fill_arr, (void*)&arr2[3]);
    th5 = thread_create(fill_arr, (void*)&arr2[4]);


    thread_join(th1, NULL);
    thread_join(th2, NULL);
    thread_join(th3, NULL);
    thread_join(th4, NULL);
    thread_join(th5, NULL);

    for (int i =0;i<5;i++) {
        printf("%d ", arr[i]);
    }

    thread_exit(NULL);
}