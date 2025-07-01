#include "parsers.h"

static int rescuer_requests_total_count = 0;

void parse_emergencies(server_context_t *ctx){
	char *filename = EMERGENCY_TYPES_CONF;
	FILE *emergencies_conf = fopen(filename, "r");					
	check_opened_file_error_log(emergencies_conf);	
	emergency_type_t ** emergency_types = callocate_emergency_types();	
	short priority;																// campi semplici da estrarre
	char emergency_desc[EMERGENCY_NAME_LENGTH];		// 										
	char* rescuer_requests_string;								// 		
	int resquers_req_number;											// campi più complessi da estrarre
	rescuer_request_t **rescuers = NULL;					// array dei rescuers dell'emergenza, lo alloco e riempio durante l'estrazione dei dati	 																																								
	
	int emergency_types_parsed_so_far = 0;				// contatori di linee e numero di emergenze
	int line_count = 0;	
	char *line = NULL;														// Leggo ogni riga del file e processo le informazioni contenute
	size_t len = 0;

	while (getline(&line, &len, emergencies_conf) != -1) {
		line_count++;

		log_and_fail_if_file_line_cant_be_processed(line, line_count, MAX_EMERGENCY_CONF_LINES, emergency_types_parsed_so_far, MAX_EMERGENCY_TYPES_COUNT, MAX_EMERGENCY_CONF_LINE_LENGTH, EMERGENCY_TYPES_CONF);
		if(check_and_log_if_line_is_empty(line, filename)) 
			continue;

		check_and_extract_simple_emergency_type_fields_from_line(filename, line_count, line, &priority, emergency_desc, rescuer_requests_string);
		if(check_and_log_if_emergency_type_already_parsed(emergency_types, emergency_desc, line_count))
			continue;

		rescuers = check_and_extract_rescuer_requests_from_string(ctx->rescuer_types, filename, line_count, rescuer_requests_string, &resquers_req_number);
		mallocate_and_populate_emergency_type(priority, emergency_desc, resquers_req_number, rescuers, emergency_types, emergency_types_parsed_so_far); // metto l'wemergenza nell'array di emergenze
		emergency_types_parsed_so_far++;
		log_event(emergency_types_parsed_so_far, EMERGENCY_PARSED, "Emergenza %s di priorità %hd con %d tipi di rescuer registrata tra i tipi di emergenze", emergency_desc, priority, resquers_req_number);
	}

	ctx -> emergency_types_count = emergency_types_parsed_so_far; // restituisco il numero di emergenze lette
	ctx -> emergency_types = emergency_types;	
}

int check_and_log_if_emergency_type_already_parsed(emergency_type_t**types, char *desc, int line_count){
	if(get_emergency_type_by_name(desc, types)){ 																																					// Controllo che non ci siano duplicati, se ci sono ignoro la riga
		log_event(line_count, DUPLICATE_EMERGENCY_TYPE_IGNORED, "L'emergenza %s è gìa stata aggiunta, ignorata", desc);
		return YES;
	}
	return NO;
}

void check_and_extract_simple_emergency_type_fields_from_line(char* filename, int line_count, char *line, short *priority, char *desc, char *requests){
	if(
		sscanf(line, EMERGENCY_TYPE_SYNTAX, desc, priority, requests) !=3 ||
		emergency_values_are_illegal(desc, *priority)
	)	{
		log_fatal_error(LINE_FILE_ERROR_STRING "%s", line_count, filename, line);
	}
}

void check_and_extract_rescuer_request_fields_from_token(char *filename, int line_count, int requests_parsed_so_far, char*token, char *name, int *count, int *time){
	if(requests_parsed_so_far > MAX_RESCUER_REQ_NUMBER_PER_EMERGENCY)		
		log_fatal_error(LINE_FILE_ERROR_STRING "Numero massimo di rescuer richiesti per una sola emergenza superato", line_count, filename);	
	if(sscanf(token, RESCUER_REQUEST_SYNTAX, name, count, time) != 3) 	
		log_fatal_error(LINE_FILE_ERROR_STRING "la richiesta del rescuer numero %d è errata", line_count, filename, requests_parsed_so_far);	
	if(rescuer_request_values_are_illegal(name, *count, *time))					
		log_fatal_error(LINE_FILE_ERROR_STRING "la richiesta del rescuer numero %d contiene valori illegali", line_count, filename, requests_parsed_so_far);	
}

int check_and_log_if_rescuer_request_already_parsed(char*filename, int line_count, rescuer_request_t **requests, char* name){
	if(get_rescuer_request_by_name(name, requests)){ 																																				
		log_event(line_count, DUPLICATE_RESCUER_REQUEST_IGNORED, "Linea %d file %s: il rescuer %s è gìa stato richesto dall' emergenza, ignorato", line_count, filename, name);
		return YES;
	}
	return NO;	
}

check_if_rescuer_requested_is_available(char *filename, int line_count, rescuer_type_t **types, char *name, int count){
	rescuer_type_t *type = get_rescuer_type_by_name(name, types);
	if(!type) log_fatal_error(LINE_FILE_ERROR_STRING "Richiesto rescuer inesistente %s", line_count, filename, name);	
	if(count > type->amount) log_fatal_error(LINE_FILE_ERROR_STRING "Numero di rescuer richiesti superiore a quelli disponibili (%d)", line_count, filename, type->amount);	
}

rescuer_request_t **check_and_extract_rescuer_requests_from_string(rescuer_type_t **rescuer_types, char *filename, int line_count, char *string_of_rescuer_requests, int *rescuer_req_number){
	rescuer_request_t **requests = callocate_resquer_requests();
	char name[MAX_RESCUER_NAME_LENGTH];			// buffer per il nome del rescuer
	int required_count, time_to_manage; 		// variabili temporanee per i campi del rescuer
	int requests_parsed_so_far = 0;					// aumenta ad ogni rescuer registrato e rappresenta anche l'indice dove mettere la richiesta nell'array
	for(char *token = strtok(string_of_rescuer_requests, RESCUER_REQUESTS_SEPARATOR); token != NULL; token = strtok(NULL, RESCUER_REQUESTS_SEPARATOR)){
		check_and_extract_rescuer_request_fields_from_token(filename, line_count, requests_parsed_so_far, token, name, &required_count, &time_to_manage);
		if(check_and_log_if_rescuer_request_already_parsed(filename, line_count, requests, name))
			continue;
		check_if_rescuer_requested_is_available(filename, line_count, rescuer_types, name, required_count);
		mallocate_and_populate_rescuer_request(name, required_count, time_to_manage, requests, rescuer_types, requests_parsed_so_far);	// Il rescuer richiesto è valido, quindi lo alloco e aggiungo alla lista di rescuer richiesti per l'emergenza
		log_event(rescuer_requests_total_count, RESCUER_REQUEST_ADDED, "Richiesta di %d unità di %s registrate a linea %d del file %s", required_count, name, line_count, filename);
		requests_parsed_so_far++;														
		rescuer_requests_total_count++;
	}
	*rescuer_req_number = requests_parsed_so_far;
	return requests;
}

int rescuer_request_values_are_illegal(char *rr_name, int required_count, int time_to_manage){
	return (
		strlen(rr_name) <= 0 || 
		required_count < MIN_RESCUER_REQUIRED_COUNT || 
		required_count > MAX_RESCUER_REQUIRED_COUNT || 
		time_to_manage < MIN_TIME_TO_MANAGE || 
		time_to_manage > MAX_TIME_TO_MANAGE
	);
}

int emergency_values_are_illegal(char *emergency_desc, short priority){
	return (
		strlen(emergency_desc) <= 0 || 
		priority < MIN_EMERGENCY_PRIORITY || 
		priority > MAX_EMERGENCY_PRIORITY
	);
}

// memory management

emergency_type_t ** callocate_emergency_types(){
	emergency_type_t **emergency_types = (emergency_type_t **)calloc(MAX_EMERGENCY_TYPES_COUNT + 1, sizeof(emergency_type_t *));
	check_error_memory_allocation(emergency_types);
	return emergency_types;
}

rescuer_request_t ** callocate_resquer_requests(){
	rescuer_request_t **rescuer_requests = (rescuer_request_t **)calloc(MAX_RESCUER_REQ_NUMBER_PER_EMERGENCY, sizeof(rescuer_request_t*));
	check_error_memory_allocation(rescuer_requests);
	return rescuer_requests;
}

void mallocate_and_populate_emergency_type(short priority, char *emergency_desc, int rescuer_req_number,	rescuer_request_t **rescuers,	emergency_type_t **emergency_types, int i){
	emergency_type_t *e = (emergency_type_t *)malloc(sizeof(emergency_type_t));			// allco l'emergency_type_t
	check_error_memory_allocation(e);
	e->emergency_desc = (char *)malloc((strlen(emergency_desc) + 1) * sizeof(char));		
	check_error_memory_allocation(e->emergency_desc);							// alloco il nome dell'emergency_type_t e lo copio
	strcpy(emergency_types[i]->emergency_desc, emergency_desc);
	e->priority = priority;																				// popolo il resto dei campi
	e->rescuers_req_number = rescuer_req_number;
	e->rescuers = rescuers;																				// assegno i rescuer richiesti
	emergency_types[i] = e;
}

void mallocate_and_populate_rescuer_request(char *rr_name, int required_count, int time_to_manage, rescuer_request_t **rescuers, rescuer_type_t **rescuer_types, int i){
	rescuer_request_t *r = (rescuer_request_t *)malloc(sizeof(rescuer_request_t));
	check_error_memory_allocation(r);
	r->type = (rescuer_type_t *)get_rescuer_type_by_name(rr_name, rescuer_types);
	r->required_count = required_count;
	r->time_to_manage = time_to_manage;
	rescuers[i] = r;
}

emergency_request_t* mallocate_and_populate_emergency_request(char* name, int x, int y, time_t d){
	emergency_request_t *e = (emergency_request_t *)malloc(sizeof(emergency_request_t));
	check_error_memory_allocation(e);
	strcpy(e->emergency_name, name);
	e->x = x;
	e->y = y;
	e->timestamp = d;
	return e;
}

void free_rescuer_requests(rescuer_request_t **rescuer_requests){
	for(int i = 0; rescuer_requests[i] != NULL; i++)
		free(rescuer_requests[i]);	// libero il puntatore al rescuer_request_t
	free(rescuer_requests);
}

void free_emergency_types(emergency_type_t **emergency_types){
	for(int i = 0; emergency_types[i] != NULL; i++){
		free(emergency_types[i]->emergency_desc);							// libero il puntatore al nome 
		free_rescuer_requests(emergency_types[i]->rescuers); 	// libero ogni rescuer_request_t
		free(emergency_types[i]);															// libero il puntatore alla singola istanza emergency_type_t 
	}
	free(emergency_types);	// libero l'array di puntatori agli emergency_types
}



