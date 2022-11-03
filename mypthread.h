// File:	mypthread_t.h

// List all group members' names: Akshat Adlakha, Justyn Cheung
// iLab machine tested on: ilab1

#ifndef MYTHREAD_T_H
#define MYTHREAD_T_H

#define _GNU_SOURCE

/* in order to use the built-in Linux pthread library as a control for benchmarking, you have to comment the USE_MYTHREAD macro */
#define USE_MYTHREAD 1

#define PSJF 1

/* include lib header files that you need here: */
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <ucontext.h>
#include <sys/time.h>
#include <signal.h>

typedef uint mypthread_t;

enum thread_state
{
	READY,
	RUNNING,
	BLOCKED,
	EXIT
};

enum thread_lock_state
{
	UNLOCKED,
	LOCKED
};

/* add important states in a thread control block */
typedef struct threadControlBlock
{
	// thread Id
	mypthread_t ID;
	// thread status
	enum thread_state status;
	// thread context
	ucontext_t *th_ct;
	// thread stack

	// thread that has yielded
	int yield;
	// thread runtime increment
	int quant;
	struct timeval birthtime;
	// And more ...
	void *val_ptr;
} tcb;

/* mutex struct definition */
typedef struct mypthread_mutex_t
{
	// YOUR CODE HERE
	atomic_flag lock;
	struct Queue *blocked_queue;

} mypthread_mutex_t;

// Feel free to add your own auxiliary data structures (linked list or queue etc...)
typedef struct QNode
{
	tcb *tcb;
	struct QNode *next;
} qnode;

typedef struct Queue
{
	struct QNode *head, *rear;
} queue;

struct QNode *newNode(tcb *item);
struct Queue *createQueue();
void enqueue(struct Queue *q, tcb *item);
tcb *dequeue(struct Queue *q);
void sortqueue(struct Queue *q);

static void schedule();

/* Function Declarations: */

/* create a new thread */
int mypthread_create(mypthread_t *thread, pthread_attr_t *attr, void *(*function)(void *), void *arg);

/* current thread voluntarily surrenders its remaining runtime for other threads to use */
int mypthread_yield();

/* terminate a thread */
void mypthread_exit(void *value_ptr);

/* wait for thread termination */
int mypthread_join(mypthread_t thread, void **value_ptr);

/* initialize a mutex */
int mypthread_mutex_init(mypthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr);

/* aquire a mutex (lock) */
int mypthread_mutex_lock(mypthread_mutex_t *mutex);

/* release a mutex (unlock) */
int mypthread_mutex_unlock(mypthread_mutex_t *mutex);

/* destroy a mutex */
int mypthread_mutex_destroy(mypthread_mutex_t *mutex);


#ifdef USE_MYTHREAD
#define pthread_t mypthread_t
#define pthread_mutex_t mypthread_mutex_t
#define pthread_create mypthread_create
#define pthread_exit mypthread_exit
#define pthread_join mypthread_join
#define pthread_mutex_init mypthread_mutex_init
#define pthread_mutex_lock mypthread_mutex_lock
#define pthread_mutex_unlock mypthread_mutex_unlock
#define pthread_mutex_destroy mypthread_mutex_destroy
#endif

#endif
