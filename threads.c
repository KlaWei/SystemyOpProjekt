#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>

#define STACKSIZE (256 * 1024) // stacksize of a thread context
#define RUNNING 0
#define READY 1
#define BLOCKED 2
#define FINISHED 3


struct control_block {
    int tid;
    int state;
    ucontext_t *t_context;
    int parent_thread_id;
    struct control_block *next;
    
}; typedef struct control_block t_cb;


struct queue {
    t_cb *first;
    t_cb *last;
}; typedef struct queue t_queue;

int t_id = 0;

// one queue for now, might add seperate for blocked & ready
t_queue *thread_q;

t_cb *currently_running;


void enqueue(int tid, ucontext_t *contx, int state) {
    t_cb *temp = (t_cb*)malloc(sizeof(t_cb));
    temp->next = NULL;
    temp->tid = tid;
    temp->state = state;
    temp->t_context = contx;
    temp->parent_thread_id = -1;

    if (state == RUNNING) {
        currently_running = temp;
    }
    if (thread_q->first == NULL) {
        thread_q->first = temp;
        thread_q->last = temp;
    }
    else {
        thread_q->last->next = temp;
        thread_q->last = temp;
    }
}


t_cb* get_nxt_ready_thread() {
    t_cb *temp = thread_q->first;
    if (thread_q->first == NULL) return NULL;
    while (temp != NULL) {
        if (temp->state == READY) break;
        temp = temp->next;
    }

    return temp;
}


t_cb* get_by_tid(int tid) {
    t_cb *temp = thread_q->first;
    while (temp != NULL) {
        if (temp->tid == tid) {
            break;
        }
        temp = temp->next;
    }

    return temp;
}


void print_queue() {
    t_cb* temp = thread_q->first;
    printf("queue\n");
    while (temp != NULL) {
        printf("tid: %d, state: %d\n", temp->tid, temp->state);
        temp=temp->next;
    }
    printf("\n");
}


// creating main thread
void libinit() {
    thread_q = (t_queue*)malloc(sizeof(t_queue));
    currently_running = NULL;
    ucontext_t *new_cntx = (ucontext_t*)malloc(sizeof(ucontext_t));

    if(getcontext(new_cntx) == -1) {
        printf("err init\n");
        exit(1);
    }

    new_cntx->uc_stack.ss_sp = malloc(STACKSIZE);
    new_cntx->uc_stack.ss_size = STACKSIZE;
    new_cntx->uc_link = NULL; // points to next thread to run
    
    
    enqueue(t_id++, new_cntx, RUNNING);
}


// creating a thread a putting it on a queue
int thread_create(void *(*func)(void *), void *arg) {
    ucontext_t *new_cntx = (ucontext_t*)malloc(sizeof(ucontext_t));

    if (getcontext(new_cntx) == -1) {
        printf("err create\n");
        exit(1);
    }
    new_cntx->uc_stack.ss_sp = malloc(STACKSIZE);
    new_cntx->uc_stack.ss_size = STACKSIZE;
    new_cntx->uc_link = NULL;

    makecontext(new_cntx, (void (*)(void))func, 1, arg);
    enqueue(t_id++, new_cntx, READY);


    return thread_q->last->tid;
}

// for now assuming that retval is NULL, ! TODO later
void thread_join(int thread_tid, void **retval) {
    t_cb *temp = get_by_tid(thread_tid);
    t_cb *parent = currently_running;
    parent->state = BLOCKED;
    currently_running = temp;
    temp->state = RUNNING;
    temp->parent_thread_id = parent->tid;

    if (swapcontext(parent->t_context, temp->t_context) == -1) {
        printf("err swap cntxt\n");
        exit(1);
    }
}

void thread_exit(void *retval) {
    int id;
    if ((id = currently_running->parent_thread_id) != -1) {
        t_cb *p = get_by_tid(id);
        p->state = READY;
    }

    t_cb *to_run = get_nxt_ready_thread();

    if (to_run == NULL) { // no ready threads
        //printf("exit\n");
        exit(0);

    }

    currently_running->state = FINISHED;
    to_run->state = RUNNING;
    currently_running = to_run;

    setcontext(currently_running->t_context);
}
