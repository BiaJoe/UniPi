#include "utils.h"

/* FUNZIONI PER ACCESSO A STRUTTURE */

rescuer_type_t * get_rescuer_type_by_name(char *name, rescuer_type_t **rescuer_types){
	for(int i = 0; rescuer_types[i] != NULL; i++){
		if(strcmp(rescuer_types[i]->rescuer_type_name, name) == 0){
			return rescuer_types[i];
		}
	}
	return NULL;
}

emergency_type_t * get_emergency_type_by_name(char *name, emergency_type_t **emergency_types){
	for(int i = 0; emergency_types[i] != NULL; i++)
		if(strcmp(emergency_types[i]->emergency_desc, name) == 0)
			return emergency_types[i];
	return NULL;
}

rescuer_request_t * get_rescuer_request_by_name(char *name, rescuer_request_t **rescuers){
	for(int i = 0; rescuers[i] != NULL; i++)
		if(strcmp(get_name_of_rescuer_requested(rescuers[i]), name) == 0)
			return rescuers[i];
	return NULL;
}

char* get_name_of_rescuer_requested(rescuer_request_t *rescuer_request){
	rescuer_type_t *rescuer_type = (rescuer_type_t *)rescuer_request->type;
	return rescuer_type->rescuer_type_name;
}

