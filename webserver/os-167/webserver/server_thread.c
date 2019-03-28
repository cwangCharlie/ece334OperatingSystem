#include "request.h"
#include "server_thread.h"
#include "common.h"

struct server {
	int nr_threads;
	int max_requests;
	int max_cache_size;
	int exiting;
	int bufferSize;
	/* add any other parameters you need */
	pthread_t* wq;
	int* rq;
	//pointer to an array of threads
	
};
int rq_start = 0;
int rq_end = 0;
int request_counter = 0;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t full = PTHREAD_COND_INITIALIZER;
pthread_cond_t empty = PTHREAD_COND_INITIALIZER;



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
	/* read file, 
	 * fills data->file_buf with the file contents,
	 * data->file_size with file size. */
	ret = request_readfile(rq);
	if (ret == 0) { /* couldn't read file */
		goto out;
	}
	/* send file to client */
	request_sendfile(rq);
out:
	request_destroy(rq);
	file_data_free(data);
}

/* entry point functions */


/*static void* start_thread(struct server *sv){
	
	while(1){
		//printf("wtfe\n");
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
	sv->exiting = 0;
	sv->bufferSize = max_requests+1;
	
	/*if (nr_threads > 0 || max_requests > 0 || max_cache_size > 0) {
		//TBD();
	}*/
	//printf("wtf\n");
	/* Lab 4: create queue of max_request size when max_requests > 0 */
	
	if(max_requests>0){
		sv->rq = (int*)malloc(sizeof(int)*(max_requests));
	}else if(max_requests<=0){
		sv->rq = NULL;
	}
	/* Lab 5: init server cache and limit its size to max_cache_size */
	pthread_mutex_lock(&lock);
	/* Lab 4: create worker threads when nr_threads > 0 */
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
		//while(1){
			//printf("wtf\n");
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
			//if(sv->exiting) pthread_exit(0);
		//}

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

	/*for(int i = 0; i<sv->nr_threads; i++){
		
		free(sv->wq[i]);
		sv->wq[i]=NULL;
	}*/

	//call pthread join for every thread each individual thread 
	//*/
	/* make sure to free any allocated resources */
	//if(sv->wq!=NULL)
		free(sv->wq);
	//	sv->wq = NULL;
	//if(sv->rq !=NULL)
		free(sv->rq);
		//sv->rq = NULL;
	//pthread_mutex_destroy(&lock);
	//pthread_cond_destroy(&full);
	//pthread_cond_destroy(&empty);
	free(sv);
	//pthread_exit(0);
}
