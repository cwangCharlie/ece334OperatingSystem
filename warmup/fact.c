#include "common.h"
#include <string.h>


int recursion(int value);

int
main(int argc, char **argv)
{
	
	if(argc > 2){
		printf("Huh?\n");	
	}else if(atoi(argv[1])==0){
		printf("Huh?\n");
	}else if((float)atoi(argv[1]) == atof(argv[1])){

		
		int value = atoi(argv[1]);
		if(value<=12){
			printf("%d\n",recursion(value));
			
		}else{
			printf("Overflow\n");		
		}
	}else{
		printf("Huh?\n");	
	}

	

	return 0;
}


int recursion(int value){

	if(value == 1){
		return value;		
	}

	return recursion(value-1)*value;
}
