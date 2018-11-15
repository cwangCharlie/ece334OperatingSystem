#include "request.h"
#include "server_thread.h"
#include "common.h"
#include <stdbool.h>
#define CASHE_MEAN 12288
int
hash(char *str)
{
    int hash = 5381;
    int c;

    while ((c = *str++))
        hash = ((hash << 5) + hash) + c; // hash * 33 + c 

    return hash;
}
struct server {
	int nr_threads;
	int max_requests;
	int max_cache_size;
	int hashtableSize;
	int exiting;
	int bufferSize;
	/* add any other parameters you need */
	pthread_t* wq;
	int* rq;
	struct wordsElement **ht;
	struct LRU* lq;
	int currentSize;
	//pointer to an array of threads
	
};
struct wordsElement{
	char* word;
	int sizeofword;
	int key;
	int refCount;
	struct file_data* data;
	struct wordsElement* next;
	struct LRUNode* listLocation;
	
}typedef wordsElement;

struct LRUNode{
	int key;
	struct LRUNode* prv;
	struct LRUNode* next;
	struct wordsElement* data;
} typedef LRUNode;

struct LRU{
	struct LRUNode* head;
	struct LRUNode* tail;
}typedef LRU;

int rq_start = 0;
int rq_end = 0;
int request_counter = 0;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t full = PTHREAD_COND_INITIALIZER;
pthread_cond_t empty = PTHREAD_COND_INITIALIZER;
pthread_mutex_t lockCache = PTHREAD_MUTEX_INITIALIZER;

struct LRUNode* buildNodeLRU(struct wordsElement* data);
struct wordsElement* initNodewE(char* wordi,struct file_data* data, int sizeofword);
void deleteSwapNode(struct LRUNode *node, struct server* sv, int command);
void pushbackLRU(struct LRUNode* pt, struct server *sv);
struct wordsElement* cache_insert(struct file_data* data, struct server* sv);
struct wordsElement* cache_lookup(struct server* sv, struct file_data* data);
struct LRUNode* cache_evict(int amountEvict, struct server *sv);
void deleteHash(struct wordsElement *node, struct server* sv);
/* static functions */

/* initialize file data */
static struct file_data *
file_data_init(void)
{
	struct file_data *data;

	data = Malloc(sizeof(struct file_data));
	data->file_name = NULL;
	data->file_buf = NULL;
	data->file_size = 0;
	return data;
}

/* free all file data */
static void
file_data_free(struct file_data *data)
{
	free(data->file_name);
	free(data->file_buf);
	free(data);
}

static void
do_server_request(struct server *sv, int connfd)
{
	int ret;
	struct request *rq;
	struct file_data *data;

	data = file_data_init();

	/* fill data->file_name with name of the file being requested */
	rq = request_init(connfd, data);
	if (!rq) {
		file_data_free(data);
		return;
	}
	
	if(sv->max_cache_size>0){
		//check to see if this file data is already in the cache
		//I assume that at this step the data 
		
		pthread_mutex_lock(&lock);
		struct wordsElement* check = cache_lookup(sv,data);
		if(check==NULL){
			pthread_mutex_unlock(&lock);
			//not found inserting into the hashtable
			/* read file, 
			* fills data->file_buf with the file contents,
			* data->file_size with file size. */
			ret = request_readfile(rq);
			if (ret == 0) { /* couldn't read file */
				goto out;
			}

			pthread_mutex_lock(&lock);
			check = cache_lookup(sv,data);
			if(check==NULL){
				
				/*if(data->file_size > CASHE_MEAN){
					//the file size is too big, do not perform insertion 
					pthread_mutex_unlock(&lock);
					request_sendfile(rq);
					goto out_0;

				}else*/ if(sv->currentSize+data->file_size <= sv->max_cache_size){
					//has space to insert
					sv->currentSize += data->file_size;
					check = cache_insert(data,sv);
					

				}else{
					//needs to perform eviction 
					//printf("full\n");
					//evict and create space first and then perfoms insertion
					int amountE = sv->max_cache_size - sv->currentSize;
					if(amountE<0){
						printf("wtf\n");
					}
					amountE = data->file_size-amountE;
					struct LRUNode *check1 = cache_evict(amountE,sv);
					if(check1 == NULL){
						pthread_mutex_unlock(&lock);
						request_sendfile(rq);
						goto out_0;
					}else{
						sv->currentSize += data->file_size;
						check = cache_insert(data,sv);
					}


				}
			}else{
				file_data_free(data);
			}


		}
		
		if(check!=NULL){
			check->refCount++;
			request_set_data(rq,check->data);
			deleteSwapNode(check->listLocation,sv,1);

		}
		pthread_mutex_unlock(&lock);

		request_sendfile(rq);
	out:
		pthread_mutex_lock(&lock);
		if (check != NULL)
		{
			
			check->refCount--;	
			
		}
		pthread_mutex_unlock(&lock);
		
		request_destroy(rq);
		if(data ==NULL)
			file_data_free(data);

		//modify the LRU and swap
	}else{
		/* reads file, 
	 	* fills data->file_buf with the file contents,
	 	* data->file_size with file size. */
		ret = request_readfile(rq);
		if (!ret)
			goto out_0;
		/* sends file to client */
		request_sendfile(rq);
	out_0:
		request_destroy(rq);
		file_data_free(data);

	}
	
}


struct wordsElement* cache_insert(struct file_data* data, struct server* sv){
	
	// this entire session is critical session
	struct wordsElement *newELement = initNodewE(data->file_name,data,data->file_size);
	int hashKey = hash(data->file_name)%sv->hashtableSize;
	newELement->key = hashKey;
	//assume that it doesn't exist
	if(sv->ht[hashKey]==NULL){
		sv->ht[hashKey] = newELement;
	}else{
		//collision
		struct wordsElement* current = sv->ht[hashKey];
		while(current->next!=NULL){
			current=current->next;
		}
		current->next = newELement;
		//newELement->prv = current;
	}
	//make a LRUNODE AND PUSH IT TO THE BACK OF THE LRU list
	struct LRUNode* newNodeLRU = buildNodeLRU(newELement);
	newELement->listLocation=newNodeLRU;
	pushbackLRU(newNodeLRU,sv);

	return newELement;

}

struct wordsElement* cache_lookup(struct server* sv, struct file_data* data){

	char* key = data->file_name;
	int hashKey = hash(key)%sv->hashtableSize;
	struct wordsElement* iter= sv->ht[hashKey];
	if(iter==NULL){
		return NULL;
	}

	while(iter!=NULL){
		if(strcmp(key,sv->ht[hashKey]->word)==0){
			return iter;
		}
		iter=iter->next;
	}
	

	return NULL;

}
struct LRUNode* cache_evict(int amountEvict, struct server *sv){
	//check to see if the reference number is 0
	//ensure that enough amount Evict is provided 
	//if not keep on the eviction process
	
	if(sv->lq==NULL){
		return NULL;
	}
	if(sv->lq->head==NULL){
		return NULL;
	}
	struct LRUNode* wasterNode = sv->lq->head;

	while(wasterNode !=NULL){
		//this is the case I have to move on to the next one
		if(wasterNode->data->refCount!=0)
			wasterNode = wasterNode->next;
		else
			break;
	}
	if(wasterNode==NULL){
		//all nodes are in use
		return NULL;
	}

	if(amountEvict<=wasterNode->data->sizeofword){
		//great no more eviction performanced
		sv->currentSize -= wasterNode->data->sizeofword;
		//delete this element in the hashtable 
		deleteHash(wasterNode->data,sv);
		deleteSwapNode(wasterNode,sv,0);
		return wasterNode;
	}
		return NULL;
	
	
	
	//delete this element in the hashtable 


	
}


/* entry point functions */


/*static void* start_thread(struct server *sv){
	
	while(1){
		pthread_mutex_lock(&lock);
		while(rq_end==rq_start){
			pthread_cond_wait(&empty,&lock);
			if(sv->exiting){
				pthread_mutex_unlock(&lock);
				return NULL;
			}
			
		};	
		int connfn = sv->rq[rq_end];
		if((rq_start-rq_end+sv->bufferSize)%sv->bufferSize==sv->bufferSize-1){
			pthread_cond_signal(&full);
		}
		rq_end =(rq_end+1) % sv->bufferSize;
		
		pthread_mutex_unlock(&lock);
		do_server_request(sv,connfn);
	}

}*/

static void* start_thread(struct server *sv){
	
	while(1){
		//printf("wtfe\n");
		pthread_mutex_lock(&lock);
		while(request_counter == 0){
			pthread_cond_wait(&empty,&lock);
			if(sv->exiting){
				pthread_mutex_unlock(&lock);
				return NULL;
			}
		};
		
		int connfn = sv->rq[rq_end];
		if(request_counter == sv->max_requests){
			pthread_cond_broadcast(&full);
		}
		rq_end =(rq_end+1) % sv->max_requests;
		request_counter -= 1;

		pthread_mutex_unlock(&lock);
		do_server_request(sv,connfn);
	}

}


struct server *
server_init(int nr_threads, int max_requests, int max_cache_size)
{
	struct server *sv;

	sv = Malloc(sizeof(struct server));
	sv->nr_threads = nr_threads;
	sv->max_requests = max_requests;
	sv->max_cache_size = max_cache_size;
	sv->hashtableSize = (int)max_cache_size/CASHE_MEAN;
	sv->exiting = 0;
	sv->bufferSize = max_requests+1;
	sv->currentSize=0;
	/*if (nr_threads > 0 || max_requests > 0 || max_cache_size > 0) {
		//TBD();
	}*/
	/* Lab 4: create queue of max_request size when max_requests > 0 */
	
	if(max_requests>0){
		sv->rq = (int*)malloc(sizeof(int)*(max_requests));
	}else if(max_requests<=0){
		sv->rq = NULL;
	}
	/* Lab 5: init server cache and limit its size to max_cache_size */
	
	if(max_cache_size>0){
		sv->ht = malloc(sizeof(struct wordsElement)* sv->hashtableSize);
		sv->lq = malloc(sizeof(struct LRU));
		for (int i = 0; i < sv->hashtableSize; ++i)
		{
			sv->ht[i] = NULL;
		}
		sv->lq->head =NULL;
		sv->lq->tail= NULL;
	}
	/* Lab 4: create worker threads when nr_threads > 0 */
	pthread_mutex_lock(&lock);

	if(nr_threads>0){
		sv->wq = (pthread_t*)malloc(sizeof(pthread_t)*nr_threads);
		for(int i = 0; i<nr_threads; i++){
			//should pass in the address of the request line value
			//sv->wq[i] = (pthread_t*)malloc(sizeof(pthread_t));
			pthread_create(&(sv->wq[i]),NULL,(void*)&start_thread, sv);
		}
	}else if(nr_threads==0){
		sv->wq = NULL;
	}
	pthread_mutex_unlock(&lock);
	
	return sv;
}

/*void
server_request(struct server *sv, int connfd)
{
	if (sv->nr_threads == 0) { // no worker threads 
		do_server_request(sv, connfd);
	} else {
		//  Save the relevant info in a buffer and have one of the
		 //  worker threads do the work. 
		//TBD();
		//while(1){
			//printf("wtf\n");
			pthread_mutex_lock(&lock);
			while((rq_start-rq_end+sv->bufferSize)%(sv->bufferSize)==(sv->bufferSize)-1){
				pthread_cond_wait(&full,&lock);
				if(sv->exiting){
					pthread_mutex_unlock(&lock);
					return;
				}
			};	
			sv->rq[rq_start] = connfd;
			if(rq_end==rq_start){
				pthread_cond_signal(&empty);
			}
			rq_start =(rq_start+1) % sv->bufferSize;
			pthread_mutex_unlock(&lock);
			//if(sv->exiting) pthread_exit(0);
		//}

	}
}*/
void
server_request(struct server *sv, int connfd)
{
	if (sv->nr_threads == 0) { // no worker threads 
		do_server_request(sv, connfd);
	} else {
		// Save the relevant info in a buffer and have one of the
		 // worker threads do the work. 
		//TBD();
		
			pthread_mutex_lock(&lock);
			while(request_counter == sv->max_requests){
				pthread_cond_wait(&full,&lock);
				if(sv->exiting){
					pthread_mutex_unlock(&lock);
					return;
				}
			};	
			
			sv->rq[rq_start] = connfd;

			if(request_counter == 0){
				pthread_cond_broadcast(&empty);
			}
			rq_start =(rq_start+1) % sv->max_requests;
			request_counter +=1;
			pthread_mutex_unlock(&lock);
		

	}
}

void
server_exit(struct server *sv)
{
	/* when using one or more worker threads, use sv->exiting to indicate to
	 * these threads that the server is exiting. make sure to call
	 * pthread_join in this function so that the main server thread waits
	 * for all the worker threads to exit before exiting. */
	
	//wake up everything else in the wait queue
	pthread_mutex_lock(&lock);
	sv->exiting = 1;
	//pthread_cond_broadcast(&full);
	pthread_cond_broadcast(&empty);
	pthread_mutex_unlock(&lock);
	for(int i = 0; i<sv->nr_threads; i++){
		pthread_join(sv->wq[i],NULL);
		//free(sv->wq[i]);
	}

	//call pthread join for every thread each individual thread 
	//*/
	/* make sure to free any allocated resources */
	//if(sv->wq!=NULL)
	free(sv->wq);
	free(sv->rq);
	
	free(sv);
	//pthread_exit(0);
}


void pushbackLRU(struct LRUNode* pt, struct server *sv){

	if(sv->lq->head==NULL){
		//FIRST ELEMENT
		sv->lq->head = pt;
		sv->lq->head->prv = NULL;
		sv->lq->head->next = NULL;
		sv->lq->tail= sv->lq->head;
	}else{
		
		sv->lq->tail->next = pt;
		pt->prv = sv->lq->tail;
		sv->lq->tail = sv->lq->tail->next;
		sv->lq->tail->next=NULL;
	}

}

struct LRUNode* buildNodeLRU(struct wordsElement* data){
	struct LRUNode *newNode = malloc(sizeof(struct LRUNode));
	newNode->data = data;
	newNode->prv = NULL;
	newNode->next=NULL;
	return newNode;
}

void deleteSwapNode(struct LRUNode *node, struct server* sv, int command){

		if (node == sv->lq->head)
		{
			//head of the node
			struct LRUNode *temp = node;
			sv->lq->head = sv->lq->head->next;
			if (sv->lq->head!=NULL)
			{
				sv->lq->head->prv =NULL;
			}else{
				//empty
				sv->lq->tail=sv->lq->head;
			}
			if(command ==0)
				free(temp);
			else
				pushbackLRU(temp,sv);
			//at this point the thread should be in the ready list

		}else if (node == sv->lq->tail)
		{
			if(command ==0){
			//end of the list
			struct LRUNode *temp =  sv->lq->tail;
			sv->lq->tail=sv->lq->tail->prv;
			sv->lq->tail->next =NULL;
			
			free(temp);
			}
			

		}else{
			//middle of the list

			struct LRUNode *before = node->prv;
			struct LRUNode *after = node->next;
			before->next = after;
			after->prv = before;
			if(command ==0)
				free(node);
			else
				pushbackLRU(node,sv);

		}

}

void deleteHash(struct wordsElement *node, struct server* sv){


		struct wordsElement* headofH = sv->ht[node->key];

		if (node == headofH)
		{
			//head of the node
			struct wordsElement *temp = node;
			sv->ht[node->key] = sv->ht[node->key]->next;
			free(temp);
			
			//at this point the thread should be in the ready list

		}else{
			//middle of the list

			struct wordsElement* before = sv->ht[node->key];
			while(before !=NULL){
				if(before->next ==node)
					break;
				before = before->next;
			}

			struct wordsElement *after = node->next;
			before->next = after;
		
			free(node);
		
		}

}


struct wordsElement* initNodewE(char* wordi,struct file_data* data, int sizeofword){


	struct wordsElement *newNode;
	//allocate space for the new node 
	newNode = (struct wordsElement*)malloc(sizeof(struct wordsElement));
	//newNode->prv =NULL;
	newNode->refCount =0;
	newNode->word = wordi;
	newNode->next = NULL;
	newNode->sizeofword = sizeofword;
	newNode->listLocation =NULL;
	newNode->data=data;


	return newNode;

} 