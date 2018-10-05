#include <assert.h>
#include <stdlib.h>
#include <ucontext.h>
#include "thread.h"
#include "interrupt.h"
#include "sorted_points.h"
#include <stdbool.h>

void
thread_stub(void (*thread_main)(void *), void *arg)
{
	Tid ret;

	thread_main(arg); // call thread_main() function with arg
	ret = thread_exit();
	// we should only get here if we are the last thread. 
	assert(ret == THREAD_NONE);
	// all threads are done, so process should exit
	exit(0);
}



/* This is the wait queue structure */
struct wait_queue {
	/* ... Fill this in Lab 3 ... */
};

/* This is the thread control block */
struct thread {
	/* ... Fill this in ... */
	Tid threadID;
  	ucontext_t threadInfo;
  	//0 is suspended
  	//1 is running
  	//2 is exit
  	int status;
  	//struct thread* next;
  	//struct thread* prv;
	Tid nextThreadID;
	Tid preThreadID;
	void *stack;
	bool exist;
  	//

};
struct ready_node {

	struct	thread *node;
	struct ready_node *prv;
	struct	ready_node *next;
};
//initialize the array
struct thread TotalList[THREAD_MAX_THREADS] = {0};
Tid currentThread;
struct ready_node *ReadyList;
//struct thread* emptyList;
struct ready_node *endOfReady;
void deleteNode(struct ready_node *node);

void pushbackRL(struct ready_node* pt);
struct ready_node* buildNodeRL();
int freed =0;
int allocaed =0;
void
thread_init(void)
{
	/* your optional code here */
	//initialize the current running thread to 0
	struct thread first;
	//saves the current context in place

	getcontext(&(first.threadInfo));

	first.threadID = 0;
	first.status =1;// one is running
	first.stack = NULL;
	first.exist = 1;
	first.preThreadID = -1;
	first.nextThreadID = -1;
	currentThread = first.threadID;
	TotalList[0] = first;

	ReadyList = NULL;
	endOfReady =NULL;

}	


Tid
thread_id()
{
	return currentThread;
}
unsigned long stack_top;
Tid
thread_create(void (*fn) (void *), void *parg)
{
	//volatile int calledsetcontext = 0;
	//ucontext_t dummy;
	struct thread* newThread;
	
	Tid ID = 1;
	while(TotalList[ID].exist == 1){
		if(ID == THREAD_MAX_THREADS-1){
			return THREAD_NOMORE;
		}
		ID++;
	}

	newThread = &TotalList[ID];
	newThread->threadID =ID;

	newThread->exist =1;
	newThread->status =0; //STATUS IS READY
	
	
	newThread->nextThreadID = -1;
	newThread->preThreadID = -1;
	getcontext(&newThread->threadInfo);


	//change the program counter to point to the first funciton that the trhead should run 
	newThread->threadInfo.uc_mcontext.gregs[REG_RIP]= (unsigned long) &thread_stub;
	//allocate a new stack
	
	newThread->stack = malloc(THREAD_MIN_STACK);
	stack_top = (unsigned long)newThread->stack -  (unsigned long) (newThread->stack)%16 - 8 + THREAD_MIN_STACK;

	if(newThread->stack == NULL){
		return THREAD_NOMEMORY;
	}
	//change the stack pointer to the top of the new stack
	
	newThread->threadInfo.uc_mcontext.gregs[REG_RSP] = (unsigned long) newThread->stack - (unsigned long) (newThread->stack)%16 - 8 + THREAD_MIN_STACK;
	newThread->threadInfo.uc_mcontext.gregs[REG_RDI] = (unsigned long) fn;
	newThread->threadInfo.uc_mcontext.gregs[REG_RSI] = (unsigned long) parg;

	//put it on the readylist
	struct ready_node *newNode = malloc(sizeof(struct ready_node));
	newNode->node = newThread;
	newNode->next = NULL;
	pushbackRL(newNode);

	return newThread->threadID;
}

Tid
thread_yield(Tid want_tid)
{

	
	//Tid ret = want_tid;
	volatile int setcontext_called = 0;
	//int err;_ok(ret)' 

	struct thread* nextThread = NULL;
	struct ready_node *iter = NULL;
	if (want_tid == THREAD_ANY)
	{
		//TAKE THE HEAD OF THE LINKED LIST
		if (ReadyList != NULL)
		{
			//pop the head of the thread
			nextThread = ReadyList->node;
		}else{
			//invalide node
			return THREAD_NONE;
		}
	}else if (want_tid == THREAD_SELF||want_tid == currentThread)
	{
		nextThread = &TotalList[currentThread];
	}else
	{
		//find the thread that is avaliable in the ready list
		iter = ReadyList;
		while(iter!=NULL){
			if (iter->node->threadID == want_tid)
			{
				break;
			}
			iter = iter->next;
		}
		if (iter == NULL)
		{
			//invalid
			return THREAD_INVALID;
		}else{
			nextThread = iter->node;
		}

	}

	//freeing prev Thread's stack if the last thread is existed
		Tid prevTID= TotalList[currentThread].preThreadID;
		if(prevTID < 0){
			
		}else{
			struct thread *prevThread = &TotalList[prevTID];
			if(!prevThread->exist){	
				free(prevThread->stack);
			}
		}
		

	//Before doing anything we should check if the next thread is killed or not


		//NOW THE NEXTTHREAD POINTS TO THE NEXT OPERATING THREAD.
		//YIELD FUNCTION NEEDS TO BE IMPLEMENTED BELOW

		//STORE THE CURRENT THREAD
		
		//put this in the ready queue
		TotalList[currentThread].status = 0;
		

		int err = getcontext(&(TotalList[currentThread].threadInfo));
		assert(!err);
		//push the current thread onto Readylist
		if (setcontext_called == 0)
		{
			if(TotalList[currentThread].exist){
				TotalList[currentThread].nextThreadID = nextThread->threadID;
				TotalList[nextThread->threadID].preThreadID = currentThread;
				if (ReadyList==NULL)
				{
					//first element
					ReadyList = buildNodeRL();
					endOfReady=ReadyList;
				}else{
					//add to the end of the ready list
					struct ready_node *newReadyT = buildNodeRL();
					pushbackRL(newReadyT);
				}
			}
	
			if(!nextThread->exist){
				currentThread = nextThread->threadID;
				thread_exit();

			}else{
				//should the second thread that's returning
				//remove from Readylist
				//printf("wtf\n");
				if(!(nextThread->threadID==0 && nextThread->status == 2)){
					if (nextThread->threadID == ReadyList->node->threadID)
					{
					//head of the node
					struct ready_node *temp =  ReadyList;
					ReadyList = ReadyList->next;
					if (ReadyList!=NULL)
					{
						ReadyList->prv =NULL;
					}else{
						//empty
						endOfReady=ReadyList;
					}
					//printf("deleteyh %d, %d\n", temp->node->threadID, (int)temp->node->exist);
					//printf("%d\n",currentThread);
					//printf("f%d\n",freed++);
					free(temp);
					//at this point the thread should be in the ready list

					}else if (nextThread->threadID==endOfReady->node->threadID)
					{
					//end of the list
					struct ready_node *temp =  endOfReady;
					endOfReady=endOfReady->prv;
					endOfReady->next =NULL;
					//printf("deleteyE %d\n", temp->node->threadID);
					//printf("f%d\n",freed++);
					free(temp);

					}else{
					//middle of the list
					struct ready_node *before = iter->prv;
					struct ready_node *after = iter->next;
					before->next = after;
					after->prv = before;
					//printf("deleteyM %d\n", iter->node->threadID);
					//printf("f%d\n",freed++);
					free(iter);
					}
				}
			}
			
			//set the current thread to be the nextthread's ID
			currentThread = nextThread->threadID; 

			//modify the status of the thread in the net list
			TotalList[currentThread].status = 1;


			//SET THE NEW THREAD
			setcontext_called = 1;
			err = setcontext(&(nextThread->threadInfo));
		}
	
		
		return TotalList[currentThread].nextThreadID; 

}

void pushbackRL(struct ready_node* pt){
	if(ReadyList ==NULL){
		//FIRST ELEMENT
		ReadyList = pt;
		ReadyList->prv = NULL;
		ReadyList->next = NULL;
		endOfReady = ReadyList;
	}else{
		
		endOfReady->next = pt;
		pt->prv = endOfReady;
		endOfReady = endOfReady->next;
		endOfReady->next=NULL;
	}

}

struct ready_node* buildNodeRL(){
	struct ready_node *newNode = malloc(sizeof(struct ready_node));
	newNode->node = &TotalList[currentThread];
	newNode->prv = NULL;
	newNode->next=NULL;
	return newNode;
}

void deleteNode(struct ready_node *node){

	
		if (node->node->threadID == ReadyList->node->threadID)
		{
			//head of the node
			struct ready_node *temp =  ReadyList;
			ReadyList = ReadyList->next;
			if (ReadyList!=NULL)
			{
				ReadyList->prv =NULL;
			}else{
				//empty
				endOfReady=ReadyList;
			}
			free(temp);
			//at this point the thread should be in the ready list

		}else if (node->node->threadID==endOfReady->node->threadID)
		{
			//end of the list
			struct ready_node *temp =  endOfReady;
			endOfReady=endOfReady->prv;
			endOfReady->next =NULL;
			free(temp);

		}else{
			//middle of the list

			struct ready_node *before = node->prv;
			struct ready_node *after = node->next;

			before->next = after;
			after->prv = before;
			free(node);

		}

	
}
Tid
thread_exit()
{
	if(ReadyList != NULL){
		struct thread* destoryT = &TotalList[currentThread];
		destoryT->exist= false;
		destoryT->status = 2;
		destoryT->threadID = -1;
		//destoryT->threadInfo = {0};
		if(destoryT->stack!=NULL){
			free(destoryT->stack);
			destoryT->stack =NULL;
		}
		thread_yield(THREAD_ANY);
	}	
	return THREAD_NONE;

	//this should exit itself which means that all elements should be cleared in that thread;
	//end with yield any


}

Tid
thread_kill(Tid tid)
{
	// GO through the ready list

	struct ready_node *itr = ReadyList;
	while(ReadyList != NULL){
		if(itr->node->threadID == tid){
			//found the thread that needs to be killed
			itr->node->exist = false;
			itr->node->status = 2;
			break;
		}
		itr = itr->next;
	}

	if(itr ==NULL){
		return THREAD_INVALID;
	}else{
		Tid threadid = itr->node->threadID;
		free(itr->node->stack);
		itr->node->stack = NULL;
		deleteNode(itr);

		return threadid;
	}


}

/*******************************************************************
 * Important: The rest of the code should be implemented in Lab 3. *
 *******************************************************************/

/* make sure to fill the wait_queue structure defined above */
struct wait_queue *
wait_queue_create()
{
	struct wait_queue *wq;

	wq = malloc(sizeof(struct wait_queue));
	assert(wq);

	TBD();

	return wq;
}

void
wait_queue_destroy(struct wait_queue *wq)
{
	TBD();
	free(wq);
}

Tid
thread_sleep(struct wait_queue *queue)
{
	TBD();
	return THREAD_FAILED;
}

/* when the 'all' parameter is 1, wakeup all threads waiting in the queue.
 * returns whether a thread was woken up on not. */
int
thread_wakeup(struct wait_queue *queue, int all)
{
	TBD();
	return 0;
}

/* suspend current thread until Thread tid exits */
Tid
thread_wait(Tid tid)
{
	TBD();
	return 0;
}

struct lock {
	/* ... Fill this in ... */
};

struct lock *
lock_create()
{
	struct lock *lock;

	lock = malloc(sizeof(struct lock));
	assert(lock);

	TBD();

	return lock;
}

void
lock_destroy(struct lock *lock)
{
	assert(lock != NULL);

	TBD();

	free(lock);
}

void
lock_acquire(struct lock *lock)
{
	assert(lock != NULL);

	TBD();
}

void
lock_release(struct lock *lock)
{
	assert(lock != NULL);

	TBD();
}

struct cv {
	/* ... Fill this in ... */
};

struct cv *
cv_create()
{
	struct cv *cv;

	cv = malloc(sizeof(struct cv));
	assert(cv);

	TBD();

	return cv;
}

void
cv_destroy(struct cv *cv)
{
	assert(cv != NULL);

	TBD();

	free(cv);
}

void
cv_wait(struct cv *cv, struct lock *lock)
{
	assert(cv != NULL);
	assert(lock != NULL);

	TBD();
}

void
cv_signal(struct cv *cv, struct lock *lock)
{
	assert(cv != NULL);
	assert(lock != NULL);

	TBD();
}

void
cv_broadcast(struct cv *cv, struct lock *lock)
{
	assert(cv != NULL);
	assert(lock != NULL);

	TBD();
}
