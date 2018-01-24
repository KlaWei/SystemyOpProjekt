#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>

#define STACKSIZE (256 * 1024) // stacksize of a thread context
#define RUNNING 0
#define READY 1
#define BLOCKED 2
#define FINISHED 3


#define LOCKED 0
#define UNLOCKED 1

const int debug = 1;

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


struct lock {
    int *lock;
    int count;
    int state;
    int tid_owner;
    int waiting_threads[50];
    int index;
    struct lock *next;

}; typedef struct lock lock_t;

struct lock_l {
    lock_t *first;
    lock_t *last;
}; typedef struct lock_l lock_list;

lock_list *locks;

int t_id = 0;

// one queue for now, might add seperate for blocked & ready
t_queue *thread_q;

t_cb *currently_running;



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

void add_to_lock_list(int *m, int cnt) {
    lock_t *new = (lock_t*)malloc(sizeof(lock_t));
    //new->waiting_threads = (t_queue)malloc(sizeof(t_queue*))
    //new->waiting_threads->first = NULL
    //new->waiting_threads->last = NULL
    for (int i=0;i<50;i++) {
        new->waiting_threads[i]=-1;
    }
    new->index = -1;
    new->count = cnt;
    new->lock = m;
    new->next = NULL;
    new->state = UNLOCKED;

    if(locks->first == NULL) {
        locks->first = new;
        locks->last = new;
    }

    else {
        locks->last->next = new;
        locks->last = new;
    }
}

lock_t* get_lock(int* m) {
    lock_t *temp = locks->first;

    while (temp != NULL) {
        if (*(temp->lock) == *m) break;
    }

    return temp;
}


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


void print_queue() {
    t_cb* temp = thread_q->first;
    printf("queue\n");
    while (temp != NULL) {
        printf("tid: %d, state: %d\n", temp->tid, temp->state);
        temp=temp->next;
    }
    printf("\n");
}

void thread_exec(void* (*func)(void*), void* arg) {
    //printf("executing!!!!\n");
    (*func)(arg);

}

int thread_lock_init(int *m, int initial_val) {
    add_to_lock_list(m, initial_val);
    return 1;
}

void thread_lock(int *l) {
    lock_t *m = get_lock(l);
    if (m->state == UNLOCKED){
        m->count--;
        if(m->count <= 0) m->state = LOCKED;
        m->tid_owner = currently_running->tid;
    }
    else {
        t_cb* blocked = currently_running;
        blocked->state = BLOCKED;
        m->waiting_threads[++m->index] = blocked->tid;
        t_cb *tmp = get_nxt_ready_thread();
        if(swapcontext(blocked->t_context, tmp->t_context) == -1) {
            printf("err swap cntxt\n");
            exit(EXIT_FAILURE);
        }
    }
}



void thread_unlock(int *l) {
    lock_t *m = get_lock(l);
    if(m->index != -1) {
        int i = 0;
        while(m->waiting_threads[i]!=-1) {
            i++;
        }
        t_cb* tmp = get_by_tid(m->waiting_threads[i]);
        tmp->state = READY;
    }
    else {
        m->count++;
    }
}

// creating main thread
void libinit() {
    thread_q = (t_queue*)malloc(sizeof(t_queue));
    thread_q->first = NULL;
    thread_q->last = NULL;
    currently_running = NULL;
    ucontext_t *new_cntx = (ucontext_t*)malloc(sizeof(ucontext_t));

    if(getcontext(new_cntx) == -1) {
        printf("err init\n");
    }

    new_cntx->uc_stack.ss_sp = malloc(STACKSIZE);
    new_cntx->uc_stack.ss_size = STACKSIZE;
    new_cntx->uc_link = NULL; // points to next thread to run
    
    
    enqueue(t_id++, new_cntx, RUNNING);
}

// nowy wątek jest tworzony i umieszczany w kolejce, ale nie wykonywany
int thread_create(void *(*func)(void *), void *arg) {
    ucontext_t *new_cntx = (ucontext_t*)malloc(sizeof(ucontext_t));

    if (getcontext(new_cntx) == -1) {
        printf("err create\n");
    }
    new_cntx->uc_stack.ss_sp = malloc(STACKSIZE);
    new_cntx->uc_stack.ss_size = STACKSIZE;
    new_cntx->uc_link = NULL;

    makecontext(new_cntx, (void (*)(void))func, 1, arg);
    enqueue(t_id++, new_cntx, READY);

    print_queue();

    return thread_q->last->tid;
}


void thread_yield() {
    t_cb* next_thr = get_nxt_ready_thread();
    currently_running->state = READY;
    t_cb* tmp = currently_running;
    next_thr->state = RUNNING;
    currently_running = next_thr;

    if (debug) printf("thread %d yielding, now running: %d\n", tmp->tid, currently_running->tid);

    if(swapcontext(tmp->t_context, currently_running->t_context) == -1) {
        printf("err swap cntxt\n");
        exit(EXIT_FAILURE);
    }
}

// for now assuming that retval is NULL, ! TODO later
void thread_join(int thread_tid, void **retval) {
    t_cb *temp = get_by_tid(thread_tid);
    t_cb *parent = currently_running;
    parent->state = BLOCKED;
    currently_running = temp;
    temp->state = RUNNING;
    temp->parent_thread_id = parent->tid;
    //printf("currently running after join: %d\n", currently_running->tid);
    //printf("old tid: %d\n", parent->tid);
    if (swapcontext(parent->t_context, temp->t_context) == -1) {
        printf("err swap cntxt\n");
    }
    //setcontext(temp->t_context);
    //print_queue();
}

void thread_exit(void *retval) {
    int id;
    if ((id = currently_running->parent_thread_id) != -1) {
        t_cb *p = get_by_tid(id);
        p->state = READY;
    }

    t_cb *to_run = get_nxt_ready_thread();

    if (to_run == NULL) { // no ready threads
        printf("exit\n");
        exit(0);

    }

    currently_running->state = FINISHED;
    to_run->state = RUNNING;
    currently_running = to_run;

    setcontext(currently_running->t_context);
}
