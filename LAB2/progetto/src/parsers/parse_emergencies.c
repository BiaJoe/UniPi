#include "parsers.h"

static int rescuer_requests_total_count = 0;

void parse_emergencies(server_context_t *ctx){
	
	FILE *emergencies_conf = fopen(EMERGENCY_TYPES_CONF, "r");					// Apro il file di configurazione
	check_opened_file_error_log(emergencies_conf);	
	emergency_type_t ** emergency_types = callocate_emergency_types();	// inizializzo l'array di emergenze dinamicamente a NULL	
	
	char emergency_desc[EMERGENCY_NAME_LENGTH];													// variabili temporanee per i campi dell'emergenza
	int resquers_req_number;		
	short priority;	
	int local_emergency_count = 0;																			// contatori di linee e numero di emergenze
	int line_count = 0;	
	char *line = NULL;																									// Leggo ogni riga del file e processo le informazioni contenute
	size_t len = 0;

	while (getline(&line, &len, emergencies_conf) != -1) {
		line_count++;

		if(local_emergency_count > MAX_EMERGENCY_TYPES_COUNT)	log_fatal_error("Numero massimo di emergenze superato nel file di configurazione: " EMERGENCY_TYPES_CONF, FATAL_ERROR_PARSING);		// Controllo che il numero massimo di emergenze non venga superato
		if(line_count > MAX_FILE_LINES)												log_fatal_error("Numero massimo di linee superato file di configurazione " EMERGENCY_TYPES_CONF, FATAL_ERROR_PARSING);						// Controllo che il numero massimo di linee non venga superato
		if(strlen(line) > MAX_LINE_LENGTH)										log_fatal_error("Riga troppo lunga nel file di configurazione: " EMERGENCY_TYPES_CONF, FATAL_ERROR_PARSING);											// Controllo la lunghezza della riga
		if (line[0] == '\n') {		//ignoro le righe vuote																																																							
			log_event(line_count, EMPTY_CONF_LINE_IGNORED, "Riga vuota ignorata: " EMERGENCY_TYPES_CONF);						
			continue;
		}

		rescuer_request_t **rescuers = callocate_resquer_requests();		 																																								// inizializzo dinamicamente l'array di rescuer di questa emergenza. sarà ridichiarato e inizializzato ad ogni ciclo e assegnato all'emergenza
		check_emergency_type_syntax_and_extract_values(line_count,line, &priority, emergency_desc, rescuers,&resquers_req_number, ctx->rescuer_types); 	// Controllo che la sintassi sia corretta ed estraggo i valori
		if(get_emergency_type_by_name(emergency_desc, emergency_types)){ 																																								// controllo se l'emergenza è già stata aggiunta
			log_event(line_count, DUPLICATE_EMERGENCY_TYPE_IGNORED, "Nome emergenza già presente, non aggiunta: " EMERGENCY_TYPES_CONF);
			free_rescuer_requests(rescuers); 															 																																								// libero la memoria allocata per i rescuer, che non mi serve per questa riga
			continue; 																										 																																								// ignoro la riga
		}
		mallocate_and_populate_emergency_type(priority,emergency_desc, resquers_req_number,rescuers,emergency_types); // metto l'wemergenza nell'array di emergenze
		local_emergency_count++;
		log_event(local_emergency_count, EMERGENCY_PARSED, "emergenza letta: " EMERGENCY_TYPES_CONF);
	}

	ctx -> emergency_types_count = local_emergency_count; // restituisco il numero di emergenze lette
	ctx -> emergency_types = emergency_types;	
}

void check_emergency_type_syntax_and_extract_values(int line_count, char *line, short *priority, char *emergency_desc, rescuer_request_t **rescuers, int *rescuer_req_number, rescuer_type_t **rescuer_types){
	char rr_string[MAX_RESCUER_REQUESTS_LENGTH]; 				// buffer per la stringa dei rescuer
	int local_rescuer_req_number = 0; 									// contatore dei rescuer
	char local_emergency_desc[EMERGENCY_NAME_LENGTH]; 	// buffer per il nome dell'emergenza
	int local_priority = MIN_EMERGENCY_PRIORITY; 
	
	if(sscanf(line, EMERGENCY_TYPE_SYNTAX, local_emergency_desc, &local_priority, rr_string) !=3)	log_fatal_error("Errore di sintassi nel file di configurazione " EMERGENCY_TYPES_CONF, FATAL_ERROR_PARSING); // estraggo il nome dell'emergenza, la priorità e la stringa dei rescuer
	if(emergency_values_are_illegal(local_emergency_desc, local_priority))												log_fatal_error("Valori illegali nel file di configurazione: " EMERGENCY_TYPES_CONF, FATAL_ERROR_PARSING);// Controllo che i valori strettamente legati all'emergenza siano validi
	
	char *token = strtok(rr_string, ";");					// tokenizzo la stringa dei rescuer
	char rr_name[MAX_RESCUER_NAME_LENGTH];			 	// buffer per il nome del rescuer
	int required_count, time_to_manage; 					// variabili temporanee per i campi del rescuer

	while(token != NULL){													// ciclo sui token della stringa dei rescuer
		if(local_rescuer_req_number > MAX_RESCUER_REQ_NUMBER_PER_EMERGENCY)													log_fatal_error("Numero massimo di rescuer richiesti per una sola emergenza superato: " EMERGENCY_TYPES_CONF, FATAL_ERROR_PARSING);	// magg. stretto perché il contatore parte da 0	.Controllo che il numero massimo di rescuer non venga superato
		if(sscanf(token, RESCUER_REQUEST_SYNTAX, rr_name, &required_count, &time_to_manage) != 3) 	log_fatal_error("Errore di sintassi nel file di configurazione " EMERGENCY_TYPES_CONF, FATAL_ERROR_PARSING);	// Controllo la sintassi del rescuer
		if(rescuer_request_values_are_illegal(rr_name, required_count, time_to_manage))							log_fatal_error("Valori illegali nel file di configurazione: " EMERGENCY_TYPES_CONF, FATAL_ERROR_PARSING);	// Controllo che i valori siano validi
		
		if(get_rescuer_request_by_name(rr_name, rescuers)){	// Controllo se il rescuer è già presente. Se non lo è, lo aggiungo																																																								
			log_event(line_count, DUPLICATE_RESCUER_REQUEST_IGNORED, "Nome rescuer richiesto più di una volta da una singola emergenza, non aggiunto: " EMERGENCY_TYPES_CONF);	
			token = strtok(NULL, ";");
			continue; // ignoro questo token
		}

		rescuer_type_t *rr_type = get_rescuer_type_by_name(rr_name, rescuer_types);
		
		if(!rr_type)													log_fatal_error("Richiesto rescuer inesistente: " EMERGENCY_TYPES_CONF, FATAL_ERROR_PARSING);	// Controllo che il rescuer_type sia valido (cioè che esista in rescuer_types)
		if(required_count > rr_type->amount)	log_fatal_error("Numero di rescuer richiesti superiore a quelli disponibili: " EMERGENCY_TYPES_CONF, FATAL_ERROR_PARSING);	// controllo che l'emergenza non chieda più rescuer di quanti ce ne siano disponibili
		
		mallocate_and_populate_rescuer_request(rr_name, required_count, time_to_manage, rescuers,rescuer_types);	// Il rescuer richiesto è valido, quindi lo alloco e aggiungo alla lista di rescuer richiesti per l'emergenza
		local_rescuer_req_number++;						// Incremento il contatore dei rescuer
		rescuer_requests_total_count++; 			// incremento il contatore totale dei rescuer richiesti
		log_event(rescuer_requests_total_count, RESCUER_REQUEST_ADDED, "rescuer richiesto aggiunto: " EMERGENCY_TYPES_CONF);
		token = strtok(NULL, ";");						// Passo al token successivo
	}

	*priority = local_priority;
	strcpy(emergency_desc, local_emergency_desc); 
	*rescuer_req_number = local_rescuer_req_number; // l'ultimo incremento lo ha reso il numero corretto
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

void mallocate_and_populate_emergency_type(short priority, char *emergency_desc, int rescuer_req_number,	rescuer_request_t **rescuers,	emergency_type_t **emergency_types){
	int i = 0;
	while(emergency_types[i] != NULL) i++; 																					// raggiungo il primo posto libero
	emergency_types[i] = (emergency_type_t *)malloc(sizeof(emergency_type_t));			// allco l'emergency_type_t
	check_error_memory_allocation(emergency_types[i]);
	
	check_error_memory_allocation(emergency_types[i]->emergency_desc);							// alloco il nome dell'emergency_type_t e lo copio
	emergency_types[i]->emergency_desc = (char *)malloc((strlen(emergency_desc) + 1) * sizeof(char));		
	strcpy(emergency_types[i]->emergency_desc, emergency_desc);
	emergency_types[i]->priority = priority;																				// popolo il resto dei campi
	emergency_types[i]->rescuers_req_number = rescuer_req_number;
	emergency_types[i]->rescuers = rescuers;																				// assegno i rescuer richiesti
}

void mallocate_and_populate_rescuer_request(char *rr_name, int required_count, int time_to_manage, rescuer_request_t **rescuers,rescuer_type_t **rescuer_types){
	int i = 0;
	while(rescuers[i] != NULL) i++; 																								// raggiungo il primo posto libero
	rescuers[i] = (rescuer_request_t *)malloc(sizeof(rescuer_request_t));
	check_error_memory_allocation(rescuers[i]);
	rescuers[i]->type = (rescuer_type_t *)get_rescuer_type_by_name(rr_name, rescuer_types);
	rescuers[i]->required_count = required_count;
	rescuers[i]->time_to_manage = time_to_manage;
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



