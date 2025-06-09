#ifndef MEMORY_H
#define MEMORY_H

#include <stdlib.h>
#include "log.h"

// FUNZIONI PER LA GESTIONE DELLA MEMORIA

// accedere ai campi 

rescuer_type_t * get_rescuer_type_by_name(char *name, rescuer_type_t **rescuer_types);
emergency_type_t * get_emergency_type_by_name(char *name, emergency_type_t **emergency_types);
rescuer_request_t * get_rescuer_request_by_name(char *name, rescuer_request_t **rescuers);
char* get_name_of_rescuer_requested(rescuer_request_t *rescuer_request);



// allocazione

rescuer_type_t ** callocate_rescuer_types();
void mallocate_and_populate_rescuer_type(char *name, int amount, int speed, int x, int y, rescuer_type_t **rescuer_types);

emergency_type_t ** callocate_emergency_types();
rescuer_request_t ** callocate_resquer_requests();
void mallocate_and_populate_emergency_type(
	short priority, 
	char *emergency_desc, 
	int rescuer_req_number,
	rescuer_request_t **rescuers,
	emergency_type_t **emergency_types
);
void mallocate_and_populate_rescuer_request(
	char *rr_name, 
	int required_count, 
	int time_to_manage, 
	rescuer_request_t **rescuers,
	rescuer_type_t **rescuer_types
);
emergency_request_t* mallocate_and_populate_emergency_request(char* name, int x, int y, time_t d);


// liberazione

void free_rescuer_requests(rescuer_request_t **rescuer_requests);
void free_rescuer_types(rescuer_type_t **rescuer_types);
void free_emergency_types(emergency_type_t **emergency_types);


#endif