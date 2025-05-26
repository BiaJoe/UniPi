
#include "memory_management.h"

// conto dei gemelli digitali, necessario per il logging
static int rescuer_digital_twins_total_count = 0;


/* FUNZIONI PER ALLOCAZIONE & LIBERAZIONE MEMORIA DI STRUTTURE */

rescuer_type_t ** callocate_rescuer_types(){
	rescuer_type_t **rescuer_types = (rescuer_type_t **)calloc((MAX_FILE_LINES + 1),  sizeof(rescuer_type_t*));
	CHECK_FOR_MEMORY_ALLOCATION_ERROR(rescuer_types);
	return rescuer_types;
}

emergency_type_t ** callocate_emergency_types(){
	emergency_type_t **emergency_types = (emergency_type_t **)calloc(MAX_EMERGENCY_TYPES_COUNT + 1, sizeof(emergency_type_t *));
	CHECK_FOR_MEMORY_ALLOCATION_ERROR(emergency_types);

	return emergency_types;
}

rescuer_request_t ** callocate_resquer_requests(){
	// alloco l'array di rescuer_requests
	rescuer_request_t **rescuer_requests = (rescuer_request_t **)calloc(MAX_RESCUER_REQ_NUMBER_PER_EMERGENCY, sizeof(rescuer_request_t*));
	CHECK_FOR_MEMORY_ALLOCATION_ERROR(rescuer_requests);
	return rescuer_requests;
}

void mallocate_and_populate_rescuer_type(
	char *name, 
	int amount, 
	int speed, int x, 
	int y, 
	rescuer_type_t **rescuer_types
){
	
	//raggiujgo il primo posto libero in rescuer_types
	int i = 0;
	while(rescuer_types[i] != NULL) i++;

	// allco il rescuer_type_t
	rescuer_types[i] = (rescuer_type_t *)malloc(sizeof(rescuer_type_t));
	CHECK_FOR_MEMORY_ALLOCATION_ERROR(rescuer_types[i]);

	// alloco il nome del rescuer_type_t e lo copio
	rescuer_types[i]->rescuer_type_name = (char *)malloc((strlen(name) + 1) * sizeof(char));
	CHECK_FOR_MEMORY_ALLOCATION_ERROR(rescuer_types[i]->rescuer_type_name);


	// copio il nome
	strcpy(rescuer_types[i]->rescuer_type_name, name);

	// popolo il resto dei campi
	rescuer_types[i]->amount = amount;
	rescuer_types[i]->speed = speed;
	rescuer_types[i]->x = x;
	rescuer_types[i]->y = y;

	// alloco i rescuer_digital_twin_t (all'inizio tutti a NULL)
	rescuer_types[i]->twins = (rescuer_digital_twin_t **)calloc(amount + 1, sizeof(rescuer_digital_twin_t*));
	CHECK_FOR_MEMORY_ALLOCATION_ERROR(rescuer_types[i]->twins);

	// alloco ogni twin e popolo i suoi campi 
	for(int j = 0; j < amount; j++){
		rescuer_types[i]->twins[j] = (rescuer_digital_twin_t *)malloc(sizeof(rescuer_digital_twin_t));
		CHECK_FOR_MEMORY_ALLOCATION_ERROR(rescuer_types[i]->twins[j]);

		rescuer_types[i]->twins[j]->id = j;
		rescuer_types[i]->twins[j]->x = x;
		rescuer_types[i]->twins[j]->y = y;
		rescuer_types[i]->twins[j]->rescuer = rescuer_types[i];
		rescuer_types[i]->twins[j]->status = IDLE;

		rescuer_digital_twins_total_count++;
		log_event(rescuer_digital_twins_total_count, RESCUER_DIGITAL_TWIN_ADDED, "Gemello digitale aggiunto: " RESCUERS_CONF);
	}
}

void mallocate_and_populate_emergency_type(
	short priority, 
	char *emergency_desc, 
	int rescuer_req_number,
	rescuer_request_t **rescuers,
	emergency_type_t **emergency_types
){
	int i = 0;
	while(emergency_types[i] != NULL) i++; // raggiungo il primo posto libero

	// allco l'emergency_type_t
	emergency_types[i] = (emergency_type_t *)malloc(sizeof(emergency_type_t));
	CHECK_FOR_MEMORY_ALLOCATION_ERROR(emergency_types[i]);

	// alloco il nome dell'emergency_type_t e lo copio
	emergency_types[i]->emergency_desc = (char *)malloc((strlen(emergency_desc) + 1) * sizeof(char));
	CHECK_FOR_MEMORY_ALLOCATION_ERROR(emergency_types[i]->emergency_desc);
	strcpy(emergency_types[i]->emergency_desc, emergency_desc);

	// popolo il resto dei campi
	emergency_types[i]->priority = priority;
	emergency_types[i]->rescuers_req_number = rescuer_req_number;

	// assegno i rescuer richiesti
	emergency_types[i]->rescuers = rescuers;

}

void mallocate_and_populate_rescuer_request(
	char *rr_name, 
	int required_count, 
	int time_to_manage, 
	rescuer_request_t **rescuers,
	rescuer_type_t **rescuer_types
){
	int i = 0;
	while(rescuers[i] != NULL) i++; // raggiungo il primo posto libero
	// allco il rescuer_request_t
	rescuers[i] = (rescuer_request_t *)malloc(sizeof(rescuer_request_t));
	CHECK_FOR_MEMORY_ALLOCATION_ERROR(rescuers[i]);

	// popolo i campi
	rescuers[i]->type = (rescuer_type_t *)get_rescuer_type_by_name(rr_name, rescuer_types);
	rescuers[i]->required_count = required_count;
	rescuers[i]->time_to_manage = time_to_manage;
}

emergency_request_t* mallocate_and_populate_emergency_request(char* name, int x, int y, time_t d){
	emergency_request_t *e = (emergency_request_t *)malloc(sizeof(emergency_request_t));
	CHECK_FOR_MEMORY_ALLOCATION_ERROR(e);
	strcpy(e->emergency_name, name);
	e->x = x;
	e->y = y;
	e->timestamp = d;
	return e;
}

void free_rescuer_requests(rescuer_request_t **rescuer_requests){
	for(int i = 0; rescuer_requests[i] != NULL; i++){
		// libero il puntatore al rescuer_request_t
		free(rescuer_requests[i]);
	}
	free(rescuer_requests);
}

void free_rescuer_types(rescuer_type_t **rescuer_types){
	for(int i = 0; rescuer_types[i] != NULL; i++){

		//libero il puntatore al nome 
		free(rescuer_types[i]->rescuer_type_name);

		// libero ogni gemello digitale
		for(int j = 0; j < rescuer_types[i]->amount; j++)
			free(rescuer_types[i]->twins[j]);

		// libero l'array di puntatori ai gemelli digitali
		free(rescuer_types[i]->twins);

		// libero il puntatore al rescuer_type_t 
		free(rescuer_types[i]);
	}

	// libero l'array di puntatori ai rescuer_types
	free(rescuer_types);
}

void free_emergency_types(emergency_type_t **emergency_types){
	for(int i = 0; emergency_types[i] != NULL; i++){

		// libero il puntatore al nome 
		free(emergency_types[i]->emergency_desc);

		// libero ogni rescuer_request_t
		free_rescuer_requests(emergency_types[i]->rescuers); 
		
		// libero il puntatore alla singola istanza emergency_type_t 
		free(emergency_types[i]);
	}
	// libero l'array di puntatori agli emergency_types
	free(emergency_types);
}