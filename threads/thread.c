#include <assert.h>
#include <stdlib.h>
#include <ucontext.h>
#include "thread.h"
#include "interrupt.h"
#include <stdbool.h>


Tid currentThread;
void fn2(void) {
	int enabled = interrupts_set(0);
	interrupts_set(enabled);
}

void
thread_stub(void (*thread_main)(void *), void *arg)
{
	//int enabled = interrupts_set(0);
	interrupts_on();
	Tid ret;

	thread_main(arg); // call thread_main() function with arg
	ret = thread_exit();
	// we should only get here if we are the last thread. 
	assert(ret == THREAD_NONE);
	//printf("%d\n",currentThread);
	// all threads are done, so process should exit
	exit(0);
}

int countw =0;
/* This is the wait queue structure */
struct wait_queue {
	/* ... Fill this in Lab 3 ... */
	struct wait_node* head;
	struct wait_node* tail;

};

struct wait_node{
	struct thread* node;
	struct wait_node* prv;
	struct wait_node* next;
};

/* This is the thread control block */
struct thread {
	/* ... Fill this in ... */
	Tid threadID;
  	ucontext_t threadInfo;
  	//0 is ready
  	//1 is running
  	//2 is exit
	//3 is suspensed
	struct wait_queue* wqt;
  	int status;
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

struct ready_node *ReadyList;
//struct thread* emptyList;
struct ready_node *endOfReady;
void deleteNode(struct ready_node *node);

void pushbackRL(struct ready_node* pt);
struct ready_node* buildNodeRL(Tid Ids);

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
	first.exist = true;
	first.preThreadID = -3;
	first.nextThreadID = -3;
	first.wqt = NULL;
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
	int enabled = interrupts_set(0);
	fn2();
	//interrupts_off();
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
	newThread->wqt=NULL;
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
	
	interrupts_set(enabled);

	return newThread->threadID;
}

Tid
thread_yield(Tid want_tid)
{

	int enabled = interrupts_set(0);
	fn2();
	//interrupts_off();
	//Tid ret = want_tid;

	//check if i am the last thread running and state is exit
	/*if(ReadyList == NULL && TotalList[currentThread].wqt == NULL){
		if(TotalList[currentThread].wqt->head ==NULL && TotalList[currentThread].exist == false && TotalList[currentThread].status != 1){
			exit(0);
		}

	}*/



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
			interrupts_set(enabled);
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
			interrupts_set(enabled);
			return THREAD_INVALID;
		}else{
			nextThread = iter->node;
		}

	}

	//freeing prev Thread's stack if the last thread is existed

		for(int i =0; i<=1024; i++){
			if(TotalList[i].status == 2 && !TotalList[i].exist){
				//take things out from the queue
				free(TotalList[i].stack);
				TotalList[i].stack = NULL;
				
			}
		}
		
	//Before doing anything we should check if the next thread is killed or not


		//NOW THE NEXTTHREAD POINTS TO THE NEXT OPERATING THREAD.
		//YIELD FUNCTION NEEDS TO BE IMPLEMENTED BELOW

		//STORE THE CURRENT THREAD
		
		int err = getcontext(&(TotalList[currentThread].threadInfo));
		assert(!err);
		//push the current thread onto Readylist
		if (setcontext_called == 0)
		{
				//put this in the ready queue
		if(TotalList[currentThread].status ==1){
			TotalList[currentThread].status = 0;
			if(TotalList[currentThread].exist){

				if(nextThread->threadID >=0)
					TotalList[currentThread].nextThreadID = nextThread->threadID;
				
				if(currentThread >= 0)
					TotalList[nextThread->threadID].preThreadID = currentThread;

				if (ReadyList==NULL)
				{
					//first element
					ReadyList = buildNodeRL(currentThread);
					endOfReady=ReadyList;
				}else{
					//add to the end of the ready list
					struct ready_node *newReadyT = buildNodeRL(currentThread);
					pushbackRL(newReadyT);
				}
			}
		}
		
			if(!nextThread->exist){
				//if current thread is kill 
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
					
					free(temp);
					//at this point the thread should be in the ready list

					}else if (nextThread->threadID==endOfReady->node->threadID)
					{
					//end of the list
					struct ready_node *temp =  endOfReady;
					endOfReady=endOfReady->prv;
					endOfReady->next =NULL;
					
					free(temp);

					}else{
					//middle of the list
					struct ready_node *before = iter->prv;
					struct ready_node *after = iter->next;
					before->next = after;
					after->prv = before;
					
					free(iter);
					}
				}
			}
			//interrupts_set(enabled);
			//set the current thread to be the nextthread's ID
			currentThread = nextThread->threadID; 

			//modify the status of the thread in the net list
			TotalList[currentThread].status = 1;


			//SET THE NEW THREAD
			setcontext_called = 1;
			//interrupts_set(enabled);
			err = setcontext(&(nextThread->threadInfo));
		}
		
		interrupts_set(enabled);
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

struct ready_node* buildNodeRL(Tid Ids){
	struct ready_node *newNode = malloc(sizeof(struct ready_node));
	newNode->node = &TotalList[Ids];
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
	int enabled = interrupts_set(0);
	fn2();
	struct thread* destoryT = &TotalList[currentThread];
	if(destoryT->wqt!=NULL){
			thread_wakeup(destoryT->wqt,1);
			free(destoryT->wqt);
			destoryT->wqt =NULL;
		}
	if(ReadyList != NULL){
		
		//put everything in the ready queue on the list
		
		destoryT->exist= false;
		destoryT->status = 2;
		//currentThread = ReadyList->node->threadID;
		thread_yield(THREAD_ANY);
	}
	interrupts_set(enabled);	
	return THREAD_NONE;

	//this should exit itself which means that all elements should be cleared in that thread;
	//end with yield any


}

Tid
thread_kill(Tid tid)
{
	// GO through the ready list
	int enabled = interrupts_set(0);
	fn2();
	struct ready_node *itr = ReadyList;
	while(ReadyList != NULL){
		if(itr->node->threadID == tid){
			//found the thread that needs to be killed
			itr->node->exist = false;
			//itr->node->status = 2;
			break;
		}
		itr = itr->next;
	}
	//check to see if this thread is in the wait queue




	if(itr ==NULL && (TotalList[currentThread].wqt ==NULL || TotalList[currentThread].wqt->head==NULL)){
		interrupts_set(enabled);
		return THREAD_INVALID;
	}else if(TotalList[currentThread].wqt != NULL){
		//Kill the thread 
		//find the thread in the wait queue
		struct wait_node* iter = TotalList[currentThread].wqt->head;
		while(iter!=NULL){
			if(iter->node->threadID == tid){
				iter->node->exist =false;
				break;
			}
			iter = iter->next;
		}
		if(iter ==NULL){
			interrupts_set(enabled);
			return THREAD_INVALID;
		}else{
			//deleteNodewq(TotalList[currentThread].wqt,iter);
			interrupts_set(enabled);
			return tid;
		}
	}else{
		Tid threadid = itr->node->threadID;
		free(itr->node->stack);
		itr->node->stack = NULL;
		//interrupts_set(enabled);
		deleteNode(itr);
		interrupts_set(enabled);
		return threadid;
	}


}

/*******************************************************************
 * Important: The rest of the code should be implemented in Lab 3. *
 *******************************************************************/

/* make sure to fill the wait_queue structure defined above */


void pushbackWQ(struct wait_queue* pt, struct wait_node* newNode){
	if( pt->head ==NULL){
		//FIRST ELEMENT
		pt->head = newNode;
		pt->head->prv = NULL;
		pt->head->next = NULL;
		pt->tail = pt->head;
	}else{
		
		/*pt->tail->next = newNode;
		newNode->prv = pt->tail;
		pt->tail = pt->tail->next;
		pt->tail->next=NULL;*/
		struct wait_node* iter = pt->head;
		for(; iter->next !=NULL; iter =iter->next){

		}
		iter->next = newNode;
		newNode->prv = iter;
		iter = iter->next;
		iter->next=NULL;
		pt->tail = iter;
	}
}

struct wait_node* buildNodeWQ(Tid Ids){
	struct wait_node *newNode = malloc(sizeof(struct wait_node));
	newNode->node = &TotalList[Ids];
	newNode->prv = NULL;
	newNode->next=NULL;
	return newNode;
}

Tid deleteNodewq(struct wait_queue *wq, struct wait_node *node){


		Tid returnTid = 0;
		struct wait_node* head = wq->head;
		//struct wait_node* end = wq->tail;
		if(head == NULL){
			//shouldn't be empty
			return -1;
		}
		if (node->node->threadID == wq->head->node->threadID)
		{
			//head of the node
			struct wait_node *temp =  wq->head;
			wq->head = wq->head->next;
			if (wq->head!=NULL)
			{
				wq->head->prv =NULL;
			}else{
				//empty
				wq->tail=wq->head;
			}
			returnTid = temp->node->threadID;
			free(temp);
			//at this point the thread should be in the ready list

		}else{
			//middle of the list

			struct wait_node *before = node->prv;
			struct wait_node *after = node->next;

			before->next = after;
			after->prv = before;
			returnTid = node->node->threadID;
			free(node);

		}

		return returnTid;

}

struct wait_queue *
wait_queue_create()
{
	struct wait_queue *wq;

	wq = malloc(sizeof(struct wait_queue));
	assert(wq);

	wq->head = NULL;
	wq->tail = NULL;

	return wq;
}

void
wait_queue_destroy(struct wait_queue *wq)
{
	//TBD();
	while(wq->head != NULL){
		struct wait_node* temp = wq->head;
		wq->head = wq->head->next;
		free(temp);
	}
	free(wq);
}

Tid
thread_sleep(struct wait_queue *queue)
{
	//TBD();
	int enabled = interrupts_set(0);
	fn2();

	volatile int donePush = 0;
	if(queue == NULL){
		interrupts_set(enabled);
		return THREAD_INVALID;
	}

	if(ReadyList == NULL){
		interrupts_set(enabled);
		return THREAD_NONE;
	}

	struct thread* nextThread = ReadyList->node;
	//suspend the caller thread put the calling thread in the wait queue
	//find the current running thread with currentthread id
	struct thread* blockedThread = &TotalList[currentThread];
	blockedThread->status = 3; //set it as suspended

	int err = getcontext(&blockedThread->threadInfo);
	assert(!err);
	//push on the queue
	if(donePush ==0){
	struct wait_node* newNode = buildNodeWQ(currentThread);
	pushbackWQ(queue,newNode);
	//nextThreadID = ReadyList->node->threadID;
	deleteNode(ReadyList);
	nextThread->status = 1;
	donePush = 1;
	currentThread = nextThread->threadID;
	err = setcontext(&(nextThread->threadInfo));
	}
	interrupts_set(enabled);
	return nextThread->threadID;
}

/* when the 'all' parameter is 1, wakeup all threads waiting in the queue.
 * returns whether a thread was woken up on not. */
int
thread_wakeup(struct wait_queue *queue, int all)
{
	//TBD();
	int enabled = interrupts_set(0);
	fn2();
	if(queue ==NULL){
		interrupts_set(enabled);
		return 0;
	}else if(queue->head == NULL){
		interrupts_set(enabled);
		return 0;
	}


	int counter = 0;
	if(all == 0){
		//remove from the suspended q to ready Q
		Tid wakeID = deleteNodewq(queue,queue->head);
		//push this thread back on the Ready Q
		if(wakeID == -1){
			printf("shouldn't go here\n");
		}else{
			struct ready_node* newNode = buildNodeRL(wakeID);
			newNode->node->status = 0;
			pushbackRL(newNode);
		}
		interrupts_set(enabled);
		return 1;
	}else if(all == 1){

		struct wait_node* node = queue->head;
		while(queue-> head!=NULL){
			Tid wakeID = deleteNodewq(queue,node);
			struct ready_node* newNode = buildNodeRL(wakeID);
			newNode->node->status = 0;
			pushbackRL(newNode);
			//node = node->next;
			node = queue->head;
			counter++;
		}
	
	}
	interrupts_set(enabled);
		return counter;
	
}

/* suspend current thread until Thread tid exits */
Tid
thread_wait(Tid tid)
{
	//TBD();
	//put the thread on the wait queue of the the tid thread
	//Thread invalid: alerts the caller that the identifier tid does not correspond to a valid thread
	//or current thread
	int enabled = interrupts_set(0);
	fn2();
	//check the status of each thread
	if(tid <0 || tid == currentThread || tid >1024){
		interrupts_set(enabled);
		return THREAD_INVALID;
	}else if(TotalList[tid].exist == false){
		interrupts_set(enabled);
		return THREAD_INVALID;
	}
	
	//volatile Tid threadexited = currentThread;
	//create the wait queue for the thread tid
	struct thread* targetThread = &TotalList[tid];
	if(targetThread->wqt == NULL){
		targetThread->wqt = malloc(sizeof(struct wait_queue));
		targetThread->wqt->head = NULL;
		targetThread->wqt->tail =NULL;
		//countw++;
		//printf("%d\n",countw);
	}
	thread_sleep(targetThread->wqt);
	

	interrupts_set(enabled);
	return tid;
}

struct lock {
	/* ... Fill this in ... */
	struct wait_queue* wqL;
	bool avaliable;
	Tid acquiredID;
};

struct lock *
lock_create()
{
	int enabled = interrupts_set(0);
	fn2();
	struct lock *lock;

	lock = malloc(sizeof(struct lock));
	assert(lock);

	//TBD();
	lock->avaliable = true;
	lock->wqL = wait_queue_create();
	lock->acquiredID = -1;
	interrupts_set(enabled);
	return lock;
}

void
lock_destroy(struct lock *lock)
{
	int enabled = interrupts_set(0);
	fn2();
	assert(lock != NULL);
	
	//TBD();

	if(lock->avaliable){
		free(lock->wqL);
		free(lock);
	}



	interrupts_set(enabled);
}

void
lock_acquire(struct lock *lock)
{
	int enabled = interrupts_set(0);
	fn2();

	assert(lock != NULL);
	//lock->avaliable = false;

	
	while(lock->avaliable !=true){
		thread_sleep(lock->wqL);
			//printf("gg\n");
		//thread_sleep(lock->wqL);
	}	
	lock->acquiredID =currentThread;
	lock->avaliable = false;	
	//printf("out\n");
	//thread_sleep(lock->wqL);
	
	//put current thread on the wait queue 
	interrupts_set(enabled);
	
	//TBD();
}

void
lock_release(struct lock *lock)
{

	int enabled = interrupts_set(0);
	fn2();
	assert(lock != NULL);

	if(!lock->avaliable){
		lock->avaliable = true;
		
		thread_wakeup(lock->wqL,1);

	}
	
	//check if the lock had been acquired by calling thread
	interrupts_set(enabled);
	
	//TBD();
}

struct cv {
	/* ... Fill this in ... */
	struct wait_queue* wqC;
};

struct cv *
cv_create()
{
	struct cv *cv;

	cv = malloc(sizeof(struct cv));
	assert(cv);

	cv->wqC = wait_queue_create();


	//TBD();

	return cv;
}

void
cv_destroy(struct cv *cv)
{
	assert(cv != NULL);

	//TBD();
	if(cv->wqC!=NULL){
		if(cv->wqC->head==NULL)
			free(cv->wqC);
		else
			printf("shouldn't come herre\n");
	}

	free(cv);
}

void
cv_wait(struct cv *cv, struct lock *lock)
{
	int enabled = interrupts_set(0);
	fn2();
	assert(cv != NULL);
	assert(lock != NULL);

	lock_release(lock);
	//suspend the calling thread
	if(lock->acquiredID == currentThread)
		thread_sleep(cv->wqC);

	lock_acquire(lock);

	interrupts_set(enabled);
	//TBD();
}

void
cv_signal(struct cv *cv, struct lock *lock)
{
	int enabled = interrupts_set(0);
	fn2();
	assert(cv != NULL);
	assert(lock != NULL);
	if(lock->acquiredID ==currentThread)
		thread_wakeup(cv->wqC,0);

	interrupts_set(enabled);
	//TBD();
}

void
cv_broadcast(struct cv *cv, struct lock *lock)
{
	int enabled = interrupts_set(0);
	fn2();
	assert(cv != NULL);
	assert(lock != NULL);

	if(lock->acquiredID ==currentThread)
		thread_wakeup(cv->wqC,1);

	interrupts_set(enabled);
	//TBD();
}
