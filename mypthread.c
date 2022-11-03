// File:	mypthread.c

// List all group members' names: Akshat Adlakha, Justyn Cheung
// iLab machine tested on: ilab1

#include "mypthread.h"

// INITAILIZE ALL YOUR VARIABLES HERE
#define STACK_SIZE SIGSTKSZ
// In miliseconds
#define DEFAULT_INTERVAL 10
#define INTERVAL_USEC(i) (i * 1000) % 1000000
#define INTERVAL_SEC(i) i / 1000

tcb *globalTCB;
ucontext_t *schedule_contex;
mypthread_t mpt_id = 0;
suseconds_t responseTime = 0;

struct Queue *rrqueue;
struct Queue *psjfqueue;
struct Queue *quitqueue;

struct itimerval timeValue;

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

		tcb *tempTCB = (tcb *)malloc(sizeof(tcb));
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
		struct timeval firstBirth;
		gettimeofday(&firstBirth, NULL);
		tempTCB->birthtime = firstBirth;
		

		globalTCB = tempTCB;
	}

	tcb *temp2TCB = (tcb *)malloc(sizeof(tcb));
	temp2TCB->ID = mpt_id;
	*thread = mpt_id;
	mpt_id++;
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
	struct timeval birth;
	gettimeofday(&birth, NULL);
	temp2TCB->birthtime = birth;

	makecontext(temp2TCB->th_ct, (void *)function, 1, arg);

	if (!PSJF)
		enqueue(rrqueue, temp2TCB);
	else
		enqueue(psjfqueue, temp2TCB);

	// create a Thread Control Block

	// create and initialize the context of this thread

	// allocate heap space for this thread's stack
	// after everything is all set, push this thread into the ready queue

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
	unblockSignalProf(&sset);
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
			mypthread_yield();
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

	if(unblockedTCB != NULL)
	{
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
	{
		sched_RR(DEFAULT_INTERVAL);
	}
	else
	{
		sched_PSJF(DEFAULT_INTERVAL);
	}

	return;
}

/* Round Robin scheduling algorithm */
static void sched_RR(int quant)
{
	// YOUR CODE HERE

	// Your own implementation of RR
	// (feel free to modify arguments and return types)

	getitimer(ITIMER_PROF, &timeValue);
	int quantum_expired = timeValue.it_value.tv_usec > 0 ? 0 : 1;

	if(PSJF)
	{
		struct itimerval remaining_time = timeValue;
	}

	stoptimer();

	if (globalTCB != NULL && globalTCB->status != BLOCKED)
	{
		if (!quantum_expired && !globalTCB->yield)
		{
			mypthread_exit(NULL);
		}
		else
		{
			globalTCB->yield = 0;
			
			if(!PSJF)
			{
				enqueue(rrqueue, globalTCB);
			}
			else
			{
				if(quantum_expired)
				{
					globalTCB->quant++;
				}
				enqueue(psjfqueue, globalTCB);
			}
		}
	}

	globalTCB = dequeue(rrqueue);
	
	if (globalTCB != NULL)
	{
		
		globalTCB->status = RUNNING;

		if (globalTCB->quant == 0)
		{
			struct timeval initialTime;
			gettimeofday(&initialTime, NULL);
			responseTime = ((initialTime.tv_sec * 1000000 + initialTime.tv_usec) - (globalTCB->birthtime.tv_sec * 1000000 + globalTCB->birthtime.tv_usec));
		}
		globalTCB->quant++;
		runtimer(quant);
		setcontext(globalTCB->th_ct);
	}
	else
	{
		if(PSJF)
		{
			globalTCB = dequeue(psjfqueue);
			runtimer(DEFAULT_INTERVAL);
			setcontext(globalTCB->th_ct);
		}
	}
}

/* Preemptive PSJF (STCF) scheduling algorithm */
static void sched_PSJF(int quant)
{
	// YOUR CODE HERE

	// Your own implementation of PSJF (STCF)
	// (feel free to modify arguments and return types)

	sortqueue(psjfqueue);
	sched_RR(quant);

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

void sortqueue(struct Queue *q)
{
	struct QNode *cue = q->head;
	while (cue == NULL)
	{
		fprintf(stdout, "NOT NULL%d->", cue->tcb->ID);
		cue = cue->next;
	}
	struct Queue *temp = makeQueue();
	struct Queue *sorted = makeQueue();
	enqueue(temp, dequeue(q));
	int flag = 0;
		printf("NOT NULL->");
	while(q->head->tcb != NULL)
	{
		while(temp->head->tcb != NULL)
		{
			printf("%d", temp->head->tcb->ID);
			if(temp->head->tcb->quant < q->head->tcb->quant
			|| (temp->head->tcb->quant == q->head->tcb->quant &&
			temp->head->tcb->birthtime.tv_usec > q->head->tcb->birthtime.tv_usec))
			{
				enqueue(sorted, dequeue(temp));
			}
			else if (temp->head->tcb->quant > q->head->tcb->quant
			|| (temp->head->tcb->quant == q->head->tcb->quant &&
			temp->head->tcb->birthtime.tv_usec < q->head->tcb->birthtime.tv_usec))
			{
				enqueue(sorted, dequeue(q));
				flag = 1;
				while(temp->head != NULL)
				{
					enqueue(sorted, dequeue(temp));
				}
			}
		}
		if(!flag)
		{
			enqueue(sorted, dequeue(q));
		}
		while(sorted->head != NULL)
		{
			enqueue(temp, dequeue(sorted));
		}
	}
	while(temp->head != NULL)
	{
		enqueue(q, dequeue(temp));
	}
	free(temp);
	free(sorted);
}

static void blockSignalProf(sigset_t *set)
{
	sigemptyset(set);
	sigaddset(set, SIGPROF);
	sigprocmask(SIG_BLOCK, set, NULL);
}

static void unblockSignalProf(sigset_t *set)
{
	sigprocmask(SIG_UNBLOCK, set, NULL);
}

static void stoptimer()
{
	// Stops timer
	timeValue.it_value.tv_sec = 0;
	timeValue.it_value.tv_usec = 0;
	if (setitimer(ITIMER_PROF, &timeValue, NULL) == -1)
	{
		printf("Error calling setitimer()");
		exit(1);
	}
}

static void runtimer(int length)
{
	timeValue.it_value.tv_sec = INTERVAL_SEC(length);
	timeValue.it_value.tv_usec = INTERVAL_USEC(length);
	timeValue.it_interval.tv_sec = 0;
	timeValue.it_interval.tv_usec = 0;
	if (setitimer(ITIMER_PROF, &timeValue, NULL) == -1)
	{
		printf("Error calling setitimer()");
		exit(1);
	}
}
