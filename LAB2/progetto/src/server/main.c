#include <stdio.h>
#include <stdlib.h> 
#include "../../include/parsers.h"


int main(){
	int rescuer_count;
	rescuer_type_t** rescuers =	parse_rescuers2(&rescuer_count);

	for(int i = 0; i < rescuer_count; i++){
		printf("rescuer %d: %s\n", i, rescuers[i]->rescuer_type_name);
		for(int j = 0; j < rescuers[i]->amount; j++){
			printf("rescuer twin %d: %d\n", i, rescuers[i]->twins[j]->id);
		}
	}	

	printf("rescuer_count: %d\n", rescuer_count);
}