#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include "point.h"
#include "sorted_points.h"
#include <math.h>
#include <stdbool.h>



struct pointNode* returnPrev(struct pointNode* head, double dis, double x, double y);
void delete(struct pointNode *head);
void printList();
/* this structure should store all the points in a list in sorted order. */
struct sorted_points {
	/* you can define this struct to have whatever fields you want. */

	struct pointNode* head;
};

/* think about where you are going to store a pointer to the next element of the
 * linked list? if needed, you may define other structures. */


struct pointNode {	
	struct point *p;
	struct pointNode *next;
	double distance;

};




struct sorted_points *
sp_init()
{
	struct sorted_points *sp;

	sp = (struct sorted_points *)malloc(sizeof(struct sorted_points));
	assert(sp);

	sp->head = NULL;


	return sp;
}

void
sp_destroy(struct sorted_points *sp)
{
	struct pointNode* listHead = sp->head;
	// loop through the structure to delete the nodes
	delete(listHead);
	free(listHead);
	free(sp);
}
// Use recursion to delete from the end of the list
void delete(struct pointNode *head){

	if (head == NULL)
	{
		/* code */
		return;
	}

	/*printList(head);
	printf("\n");
*/
	if(head->next != NULL){
		delete(head->next);
	}

	free(head->p);
	free(head->next);
	return;

}

int
sp_add_point(struct sorted_points *sp, double x, double y)
{
	struct pointNode* newNode;
	newNode = (struct pointNode *)malloc(sizeof(struct pointNode));
	if(newNode == NULL){
		free(newNode);
		return 0;
	}
	
	//fill out the structure 
	newNode-> p = (struct point *)malloc(sizeof(struct point));
	if(newNode->p == NULL){
		free(newNode->p);
		free(newNode);
		return 0;
	}

	newNode->p->x = x;
	newNode->p->y = y;
	newNode->distance = sqrt(pow(x,2.0)+pow(y,2.0));
	newNode->next = NULL;

	//find the proper place to insert
	struct pointNode* pre = returnPrev(sp->head,newNode->distance,x,y);

	if (pre == NULL)
	{
		//first NOde
		sp->head =  newNode;
		//newNode = NULL;
	}else if(pre == sp->head){
		
		struct pointNode* temp = sp->head;
		sp->head = newNode;
		newNode->next = temp;
		
		//newNode = NULL;
		
	}else{
		struct pointNode* temp = pre->next;
		pre->next = newNode;
		newNode->next = temp;
		//temp = NULL;
		
		
	}

	//pre = NULL;
/*	printList(sp->head);
	printf("\n");*/

	
	return 1;
}

struct pointNode* returnPrev(struct pointNode* head, double dis, double x, double y){
	//check if dummy is null
	

	if(head ==  NULL)
		return NULL;

	struct pointNode* current = head;
	struct pointNode* nextNode=head->next;
	//printf("%d\n", nextNode);
	
	while(nextNode!=NULL){

		if(dis > nextNode->distance){
			//keeps incrementing 

			current = current->next;
			nextNode = nextNode->next;		
		}else if(nextNode->distance == dis){

			if(x>nextNode->p->x){
				//keep incrementing
				current = current->next;
				nextNode = nextNode->next;	
					
			}else if(x == nextNode->p->x){

				if(y>nextNode->p->y){
					current = current->next;
					nextNode = nextNode->next;
				}else{
					return current;
				}
			

			}else{
				return current;
			}
			
		}else{
			//return the previous pointer, location found 
			return current;		
			
		}
		

	}
	
	
	return current;


}


int
sp_remove_first(struct sorted_points *sp, struct point *ret)
{
	if(sp->head == NULL)
		return 0;
	struct pointNode* removeNode = sp->head;
	struct pointNode* afterNode = sp->head->next;

	sp->head = afterNode;
	ret->x = removeNode->p->x;
	ret->y = removeNode->p->y;
	free(removeNode->p);
	free(removeNode);
	return 1;
	
}

int
sp_remove_last(struct sorted_points *sp, struct point *ret)
{
	if(sp->head == NULL)
		return 0;	
	/*printList(sp->head);
	printf("\n");*/
	struct pointNode* current = sp->head->next;
	struct pointNode* prev = sp->head;
	if (current == NULL)
	{
		//only one element
		/* code */
		ret->x = prev->p->x;
		ret->y = prev->p->y;
		sp->head = NULL;
		free(prev->p);
		free(prev);
		return 1;
	}

	while(current->next != NULL){
		current = current->next;
		prev = prev->next;
	}
	

	ret->x = current->p->x;
	ret->y = current->p->y;
	prev->next = NULL;
	free(current->p);
	free(current);

	/*printList(sp->head);
	printf("\n");*/
	
	return 1;
}

int
sp_remove_by_index(struct sorted_points *sp, int index, struct point *ret)
{
	/*printList(sp->head);
	printf("\n");*/
	if(sp->head == NULL)
		return 0;


	struct pointNode* removeNode = sp->head->next;
	struct pointNode* prev = sp->head;
	int counter = 1;	

	while(index>counter){
		counter++;
		if(removeNode->next == NULL){
			return 0;		
		}
		removeNode = removeNode->next;
		prev = prev->next;
		
	}
	if(index == 0){
		//if head
		ret->x = prev->p->x;
		ret->y = prev->p->y;
		sp->head = removeNode;
		free(prev->p);
		free(prev); 

	}else{
		ret->x = removeNode->p->x;
		ret->y = removeNode->p->y;
		prev->next = removeNode->next;
		free(removeNode->p);
		free(removeNode);
	}
	/*printList(sp->head);
	printf("\n");*/
	return 1;
}

int
sp_delete_duplicates(struct sorted_points *sp)
{

	//printList(sp->head);
	if(sp->head == NULL){
		return 0;
	}	

	struct pointNode* current = sp->head->next;
	struct pointNode* prev = sp->head;
	int counter = 0;	
	while(current!=NULL){
		bool deleted = false;
		if(current->p->x == prev->p->x && current->p->y == prev->p->y){
			//delete the node current;
			counter++;
			struct pointNode* temp = current->next;

			prev->next = temp;
			current->next=NULL;
			free(current->p);
			free(current);
			current = temp;
			deleted = true;
			
		}
		if(!deleted){
			//increment
			current = current->next;
			prev = prev->next;
		}
				

	
	}
	
	//printList(sp->head);
	//printf("%d\n", counter);
	return counter;
}

/*void printList(struct pointNode* head){
    
	while(head!=NULL){

		printf("%f,%f\n", head->p->x,head->p->y);
		head =head->next;
	}



}
*/
