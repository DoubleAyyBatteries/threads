// File:	mypthread.c

// List all group members' names: Akshat Adlakha, Justyn Cheung
// iLab machine tested on: ilab1

#include "mypthread.h"

// INITAILIZE ALL YOUR VARIABLES HERE
#define STACK_SIZE SIGSTKSZ
// In miliseconds
#define DEFAULT_INTERVAL 10

tcb *globalTCB;
ucontext_t *schedule_contex;
mypthread_t mpt_id = 0;

struct Queue *rrqueue;
struct Queue *psjfqueue;
struct Queue *quitqueue;

// int numthreads = 0;
// YOUR CODE HERE

/* create a new thread */
int mypthread_create(mypthread_t *thread, pthread_attr_t *attr, void *(*function)(void *), void *arg)
{
	// YOUR CODE HERE
	if (schedule_contex == NULL)
	{
		struct sigaction sig_alm;
		sig_alm.sa_flags = 0;
		sig_alm.sa_handler = handler;
		sigfillset(&sig_alm.sa_mask);
		sigdelset(&sig_alm.sa_mask, SIGPROF);

		if (sigaction(SIGPROF, &sig_alm, NULL) == -1)
		{
			printf("Unable to catch SIGALRM");
			exit(1);
		}

		schedule_contex = (struct ucontext_t *)malloc(sizeof(struct ucontext_t));
		schedule_contex->uc_link = NULL;
		// void *schedule_contex_stack = malloc(STACK_SIZE); (Either?)
		schedule_contex->uc_stack.ss_sp = malloc(STACK_SIZE);
		schedule_contex->uc_stack.ss_size = STACK_SIZE;
		schedule_contex->uc_stack.ss_flags = 0;
		getcontext(schedule_contex);
		makecontext(schedule_contex, schedule, 0);

		if (!PSJF)
			rrqueue = makeQueue();
		else
			psjfqueue = makeQueue();

		quitqueue = makeQueue();

		tcb *tempTCB = (struct tcb *)malloc(sizeof(tcb));
		tempTCB->ID = mpt_id;
		mpt_id++;
		tempTCB->status = RUNNING;
		tempTCB->quant = 0;
		tempTCB->th_ct = (struct ucontext_t *)malloc(sizeof(struct ucontext_t));

		void *tempstack = malloc(STACK_SIZE);
		if (tempstack == NULL)
		{
			printf("Error allocating main stack");
			exit(1);
		}

		tempTCB->th_ct->uc_link = NULL;
		tempTCB->th_ct->uc_stack.ss_sp = tempstack;
		tempTCB->th_ct->uc_stack.ss_size = STACK_SIZE;
		tempTCB->th_ct->uc_stack.ss_flags = 0;

		globalTCB = tempTCB;
	}

	tcb *temp2TCB = (struct tcb *)malloc(sizeof(tcb));
	temp2TCB->ID = mpt_id;
	*thread = mpt_id;
	mpt_id++;
	// numthreads++;
	temp2TCB->status = READY;
	temp2TCB->quant = 0;
	temp2TCB->th_ct = (struct ucontext_t *)malloc(sizeof(struct ucontext_t));

	if (getcontext(temp2TCB->th_ct) < 0)
	{
		printf("Error: Cannot get context");
		exit(1);
	}

	void *temp2stack = malloc(STACK_SIZE);

	if (temp2stack == NULL)
	{
		printf("Failed to allocate thread stack");
		exit(1);
	}

	temp2TCB->th_ct->uc_link = schedule_contex;
	temp2TCB->th_ct->uc_stack.ss_sp = temp2stack;
	temp2TCB->th_ct->uc_stack.ss_size = STACK_SIZE;
	temp2TCB->th_ct->uc_stack.ss_flags = 0;

	makecontext(temp2TCB->th_ct, (void *)function, 1, arg);

	if (!PSJF)
		enqueue(rrqueue, temp2TCB);
	else
		enqueue(psjfqueue, temp2TCB);

	// create a Thread Control Block

	// create and initialize the context of this thread

	// allocate heap space for this thread's stack
	// after everything is all set, push this thread into the ready queue

	// int id = *((int*) thread);

	return 0;
};

/* current thread voluntarily surrenders its remaining runtime for other threads to use */
int mypthread_yield()
{
	// YOUR CODE HERE

	// change current thread's state from Running to Ready
	// save context of this thread to its thread control block
	// switch from this thread's context to the scheduler's context
	globalTCB->status = READY;
	globalTCB->yield = 1;
	swapcontext(globalTCB->th_ct, schedule_contex);
	return 0;
};

/* terminate a thread */
void mypthread_exit(void *value_ptr)
{
	// YOUR CODE HERE

	// preserve the return value pointer if not NULL
	// deallocate any dynamic memory allocated when starting this thread
	sigset_t sset;
	blockSignalProf(&sset);

	free(globalTCB->th_ct->uc_stack.ss_sp);
	free(globalTCB->th_ct);
	globalTCB->status = EXIT;
	globalTCB->val_ptr = value_ptr;
	enqueue(quitqueue, globalTCB);

	// return;
};

/* Wait for thread termination */
int mypthread_join(mypthread_t thread, void **value_ptr)
{
	sigset_t sset;
	blockSignalProf(&sset);
	// YOUR CODE HERE

	// wait for a specific thread to terminate
	// deallocate any dynamic memory created by the joining thread
	tcb *tempTCB = NULL;
	qnode *tempNode;

	if (!PSJF)
	{
		tempNode = rrqueue->head;
		while (tempNode != NULL && tempNode->tcb->ID != thread)
		{
			tempNode = tempNode->next;
		}
		if (tempNode == NULL)
		{
			tempNode = quitqueue->head;
			while (tempNode != NULL && tempNode->tcb->ID != thread)
			{
				tempNode = tempNode->next;
			}
		}
	}
	else
	{
		tempNode = psjfqueue->head;
		while (tempNode != NULL && tempNode->tcb->ID != thread)
		{
			tempNode = tempNode->next;
		}
		if (tempNode == NULL)
		{
			tempNode = quitqueue->head;
			while (tempNode != NULL && tempNode->tcb->ID != thread)
			{
				tempNode = tempNode->next;
			}
		}
	}

	if (tempNode != NULL && tempNode->tcb->ID == thread)
		tempTCB = tempNode->tcb;

	unblockSignalProf(&sset);

	if (tempTCB != NULL)
	{
		while (tempTCB->status != EXIT)
		{
			worker_yield();
		}
		if (value_ptr != NULL)
			*value_ptr = tempTCB->val_ptr;
	}

	return 0;
};

/* initialize the mutex lock */
int mypthread_mutex_init(mypthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr)
{
	// YOUR CODE HERE

	// initialize data structures for this mutex
	sigset_t sset;
	blockSignalProf(&sset);

	atomic_flag_clear(&mutex->lock);
	mutex->blocked_queue = makeQueue();

	unblockSignalProf(&sset);
	return 0;
};

/* aquire a mutex lock */
int mypthread_mutex_lock(mypthread_mutex_t *mutex)
{
	// YOUR CODE HERE

	// use the built-in test-and-set atomic function to test the mutex
	// if the mutex is acquired successfully, return
	// if acquiring mutex fails, put the current thread on the blocked/waiting list and context switch to the scheduler thread
	sigset_t sset;
	blockSignalProf(&sset);

	while (atomic_flag_test_and_set(&mutex->lock) == LOCKED)
	{
		globalTCB->status = BLOCKED;
		enqueue(mutex->blocked_queue, globalTCB);

		unblockSignalProf(&sset);
		swapcontext(globalTCB->th_ct, schedule_contex);
	}

	unblockSignalProf(&sset);
	return 0;
};

/* release the mutex lock */
int mypthread_mutex_unlock(mypthread_mutex_t *mutex)
{
	// YOUR CODE HERE

	// update the mutex's metadata to indicate it is unlocked
	// put the thread at the front of this mutex's blocked/waiting queue in to the run queue
	sigset_t sset;
	blockSignalProf(&sset);

	atomic_flag_clear(&mutex->lock);
	tcb *unblockedTCB = dequeue(mutex->blocked_queue);

	if(unblockedTCB != NULL){
		unblockedTCB->status = READY;

		if(!PSJF)
			enqueue(rrqueue, unblockedTCB);
		else
			enqueue(psjfqueue, unblockedTCB);
	}

	unblockSignalProf(&sset);
	return 0;
};

/* destroy the mutex */
int mypthread_mutex_destroy(mypthread_mutex_t *mutex)
{
	// YOUR CODE HERE

	// deallocate dynamic memory allocated during mypthread_mutex_init
	qnode *tempNode = mutex->blocked_queue->head;
	qnode *prev;

	while (tempNode != NULL)
	{
		prev = tempNode;
		tempNode = tempNode->next;
		free(prev);
	}

	free(mutex->blocked_queue);
	return 0;
};

/* scheduler */
static void schedule()
{
	// YOUR CODE HERE

	// each time a timer signal occurs your library should switch in to this context

	// be sure to check the SCHED definition to determine which scheduling algorithm you should run
	//   i.e. RR, PSJF or MLFQ
	if(!PSJF)
		sched_rr(DEFAULT_INTERVAL);
	else
		sched_PSJF(DEFAULT_INTERVAL);

	return;
}

/* Round Robin scheduling algorithm */
static void sched_RR(int quant)
{
	// YOUR CODE HERE

	// Your own implementation of RR
	// (feel free to modify arguments and return types)
	

	return;
}

/* Preemptive PSJF (STCF) scheduling algorithm */
static void sched_PSJF()
{
	// YOUR CODE HERE

	// Your own implementation of PSJF (STCF)
	// (feel free to modify arguments and return types)

	return;
}

/* Preemptive MLFQ scheduling algorithm */
/* Graduate Students Only */
static void sched_MLFQ()
{
	// YOUR CODE HERE

	// Your own implementation of MLFQ
	// (feel free to modify arguments and return types)

	return;
}

static void handler()
{
	if (globalTCB != NULL)
		swapcontext(globalTCB->th_ct, schedule_contex);
	else
		setcontext(schedule_contex);
}

// Feel free to add any other functions you need

// YOUR CODE HERE
struct QNode *newNode(tcb *tcb)
{
	struct QNode *temp = (struct QNode *)malloc(sizeof(struct QNode));
	temp->tcb = tcb;
	temp->next = NULL;
	return temp;
}

struct Queue *makeQueue()
{
	struct Queue *q = (struct Queue *)malloc(sizeof(struct Queue));
	q->head = q->rear = NULL;
	return q;
}

void enqueue(struct Queue *q, tcb *tcb)
{
	if (tcb != NULL)
	{
		struct QNode *temp = newNode(tcb);
		if (q->rear == NULL)
		{
			q->head = q->rear = temp;
			return;
		}
		q->rear->next = temp;
		q->rear = temp;
	}
}

tcb *dequeue(struct Queue *q)
{
	if (q->head == NULL)
		return NULL;

	struct QNode *node = q->head;
	q->head = q->head->next;
	tcb *tcb = node->tcb;

	if (q->head == NULL)
		q->rear = NULL;

	free(node);
	return tcb;
}