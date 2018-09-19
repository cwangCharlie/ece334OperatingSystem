#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include "wc.h"
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <stdbool.h>


//https://stackoverflow.com/questions/7666509/hash-function-for-string
//hash function is from Dan Bernstein. on stack overflow

unsigned long
hash(char *str)
{
    unsigned long hash = 5381;
    int c;

    while ((c = *str++))
        hash = ((hash << 5) + hash) + c; // hash * 33 + c 

    return hash;
}



struct wordsElement{
	char* word;
	long sizeofword;
	unsigned long key;
	int counts;
	struct wordsElement* next;
	
}typedef wordsElement;

struct wc {
	/* you can define this struct to have whatever fields you want. */
	struct wordsElement** wcArray;
	long size; 

};

// data structure description: the wc pointer points to an array of the elements. 
struct wordsElement* initNode(char* wordi,unsigned long key, long sizeofword){


	struct wordsElement *newNode;
	//allocate space for the new node 
	newNode = (struct wordsElement*)malloc(sizeof(struct wordsElement));

	newNode->word = wordi;
	newNode->counts = 1;
	newNode->next = NULL;
	newNode->sizeofword = sizeofword;
	newNode->key = key;
	//printf("%s\n", newNode->word);

	//Htable->wcArray[key] = newNode;

	return newNode;

} 

void delete(struct wordsElement *head){

	if (head == NULL)
	{
		
		return;
	}


	if(head->next != NULL){
		delete(head->next);
	}

	free(head->word);
	free(head->next);
	return;

}



struct wc *
wc_init(char *word_array, long size)
{
	struct wc *wc;

	wc = (struct wc *)malloc(sizeof(struct wc));
	assert(wc);

	wc->wcArray = (struct wordsElement**)malloc(sizeof(struct wordsElement*)*size);
	//initalize all nodes to null
	wc->size = size;

	for (long i = 0; i < size; ++i)
	{
		
		wc->wcArray[i]=NULL;

	}

	//int countTotal=0;

	//loop through the word_array to obtain each word. 
	for (char* i = word_array; *i != '\0'; ++i)
	{
		

		char* j = i;
		char* dummy;
		int counter = 0;


		while (!isspace(*j))
		{
			counter++;
			j++;
		}


		//keep track of the total character
	

		dummy = (char*)malloc(sizeof(char)*(counter+1));


		strncpy(dummy,i,counter);
		dummy[counter] = '\0';
		//printf("%s\n",dummy);

		if (strcmp(dummy,"")==0)
		{
			/* code */
		}else{



		unsigned long index = hash(dummy) % size;
		struct wordsElement* currentHash = wc->wcArray[index];


		//check to see if this node exist first
		 if(currentHash == NULL)
		 {
		 	// newNode needs to be inserted
		 	wc->wcArray[index] = initNode(dummy,index,counter);
		 	//printf("%s\n",currentHash->word);
	

		 }else if(strcmp(currentHash->word,dummy) == 0){

		 	//same word
		 			currentHash->counts+=1;
		 	
		 }else if(strcmp(currentHash->word,dummy)!=0)
		 {
		 		//linked list 
		 		bool exist = false;
		 		struct wordsElement* pre = currentHash;

		 		currentHash = currentHash->next;
		 		while(pre->next !=NULL){
		 			if(strcmp(currentHash->word,dummy) == 0)
		 			{
		 			
		 			currentHash->counts+=1;
		 			exist=true;
		 			break;

		 			}
		 			currentHash = currentHash->next;
		 			pre = pre->next;
		 		}
		 		if (exist ==false)
		 		{
		 			struct wordsElement* nextNode = initNode(dummy,index,counter);
		 			pre->next = nextNode;
		 		}
		 		



			}
		 	
		}


		 
		 //free(dummy);


		 i = j;
		 

	}

	return wc;
}

void
wc_output(struct wc *wc)
{
	for (long i = 0; i < wc->size; ++i)
	{

		struct wordsElement* currentHash = wc->wcArray[i];

			//print the linked list at corresponding place

			while(currentHash!=NULL){
				//printf("wtf\n");
				//printf("%lu\n",i);
				printf("%s:%d\n", currentHash->word, currentHash->counts);
				currentHash = currentHash->next;
			}



	}

	
}

void
wc_destroy(struct wc *wc)
{
	

	for (long i = 0; i < wc->size; ++i)
	{
		

		struct wordsElement* currentHash = wc->wcArray[i];

		if (currentHash==NULL)
		{
			 //code 
			//printf("\n");
		}else{
			//print the linked list at corresponding place
				delete(currentHash);

		}

	}
	free(wc->wcArray);
	free(wc);
}

