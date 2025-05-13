#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <ctype.h>
#include "../include/parsers.h"

emergency_type_t** parse_emergencies(int *emergency_count, rescuer_type_t **rescuer_types){
	// Apro il file di configurazione
	FILE *emergencies_conf = fopen(EMERGENCY_TYPES_CONF, "r");
	if(!emergencies_conf) { 
		perror("Errore apertura file di configurazione " EMERGENCY_TYPES_CONF);
		exit(EXIT_FAILURE);
	}

	// inizializzo l'array di emergenze dinamicamente a NULL
	emergency_type_t **  emergency_types = init_emergency_types();

	// inizializzo i rescuer
	rescuer_request_t **rescuers = init_resquer_requests();

	// variabili temporanee per i campi dell'emergenza
	char emergency_desc[EMERGENCY_NAME_LENGTH]; 
	int resquers_req_number;		
	short priority;

	// contatori di linee e numero di emergenze
	int local_emergency_count = 0;
	int line_count = 0;

	// Leggo ogni riga del file e processo le informazioni contenute
	char *line = NULL;
	size_t len = 0;

	while (getline(&line, &len, emergencies_conf) != -1) {
		line_count++;

		// Controllo che il numero massimo di linee non venga superato
		if(line_count > MAX_FILE_LINES){
			perror("Numero massimo di linee superato file di configurazione " EMERGENCY_TYPES_CONF);
			exit(EXIT_FAILURE);
		}

		IGNORE_EMPITY_LINES();

		// Controllo la lunghezza della riga
		if(strlen(line) > MAX_LINE_LENGTH){
			perror("Riga troppo lunga file di configurazione " EMERGENCY_TYPES_CONF);
			exit(EXIT_FAILURE);
		}

		// Controllo che la sintassi sia corretta ed estraggo i valori
		check_emergency_type_syntax_and_extract_values(
			line, 
			&priority, 
			emergency_desc, 
			rescuers,
			&resquers_req_number,
			rescuer_types
		);

		if(get_emergency_type_by_name(emergency_desc, emergency_types)){
			perror("Presenza di duplicati nel file di configurazione " EMERGENCY_TYPES_CONF);
			exit(EXIT_FAILURE);
		}

		allocate_emergency_type(
			priority,
			emergency_desc, 
			resquers_req_number,
			emergency_types
		);

		local_emergency_count++;
		
		// adesso i valori sono stati estratti e sono validi
		

	}

}

emergency_type_t ** init_emergency_types(){
	// alloco l'array di emergenze
	emergency_type_t **emergency_types = (emergency_type_t **)calloc(MAX_EMERGENCY_COUNT + 1, sizeof(emergency_type_t *));
	if(!emergency_types) {
		perror("Errore allocazione memoria emergenza_types");
		exit(EXIT_FAILURE);
	}

	return emergency_types;
}

rescuer_request_t ** init_resquer_requests(){
	// alloco l'array di rescuer_requests
	rescuer_request_t **rescuer_requests = (rescuer_request_t **)calloc(MAX_RESCUER_REQ_NUMBER_PER_EMERGENCY, sizeof(rescuer_request_t*));
	if(!rescuer_requests) {
		perror("Errore allocazione memoria rescuer_requests");
		exit(EXIT_FAILURE);
	}

	return rescuer_requests;
}

// si occupa di controllare la sintassi e di estrarre i valori di UNA riga
void check_emergency_type_syntax_and_extract_values(
	char *line, 
	short *priority, 
	char *emergency_desc, 
	rescuer_request_t **rescuers,
	int *rescuer_req_number,
	rescuer_type_t **rescuer_types
){
	char rr_string[MAX_LINE_LENGTH]; // buffer per la stringa dei rescuer
	int local_rescuer_req_number = 0; // contatore dei rescuer
	char local_emergency_desc[EMERGENCY_NAME_LENGTH]; // buffer per il nome dell'emergenza
	int local_priority; // buffer per la priorità

	// estraggo il nome dell'emergenza, la priorità e la stringa dei rescuer
	if(sscanf(line, EMERGENCY_TYPE_SYNTAX, local_emergency_desc, &local_priority, rr_string) !=3){
		perror("Errore di sintassi file di configurazione " EMERGENCY_TYPES_CONF);
		exit(EXIT_FAILURE);
	}

	// Controllo che i valori strettamente legati all'emergenza siano validi
	if(emergency_values_are_illegal(local_emergency_desc, local_priority)){
		perror("Valori illegali file di configurazione " EMERGENCY_TYPES_CONF);
		exit(EXIT_FAILURE);
	}

	// tokenizzo la stringa dei rescuer
	char *token = strtok(rr_string, ";");
	char rr_name[MAX_RESCUER_NAME_LENGTH]; // buffer per il nome del rescuer
	int required_count, time_to_manage; // variabili temporanee per i campi del rescuer

	while(token != NULL){
		
		//Controllo che il numero massimo di rescuer non venga superato
		if(local_rescuer_req_number > MAX_RESCUER_REQ_NUMBER_PER_EMERGENCY){ // magg. stretto perché il contatore parte da 0
			perror("Numero massimo di rescuer richiesti per una sola emergenza superato " EMERGENCY_TYPES_CONF);
			exit(EXIT_FAILURE);
		}

		// Controllo la sintassi del rescuer
		if(
			sscanf(
				token, 
				RESCUER_REQUEST_SYNTAX, 
				rr_name, 
				required_count, 
				time_to_manage
			) != 3
		){
			perror("Errore di sintassi file di configurazione " EMERGENCY_TYPES_CONF);
			exit(EXIT_FAILURE);
		}

		// Controllo che i valori siano validi
		if(
			strlen(rr_name) <= 0 || 
			required_count <= 0 || 
			time_to_manage <= 0
		){
			perror("Valori illegali file di configurazione " EMERGENCY_TYPES_CONF);
			exit(EXIT_FAILURE);
		}

		// Controllo se il rescuer è già presente. Se non lo è, lo aggiungo
		if(get_rescuer_request_by_name(rr_name, rescuers)){
			perror("Nome rescuer già presente nella lista di rescuer richiesti per l'emergenza " EMERGENCY_TYPES_CONF);
			exit(EXIT_FAILURE);
		}

		// Controllo che il rescuer_type sia valido (cioè che esista in rescuer_types)
		if(!get_rescuer_type_by_name(rr_name, rescuer_types)){
			perror("Richiesto rescuer non presente nel file di configurazione " EMERGENCY_TYPES_CONF);
			exit(EXIT_FAILURE);
		}

		// Il rescuer richiesto è valido, quindi lo alloco e aggiungo alla lista di rescuer richiesti per l'emergenza
		allocate_rescuer_request(
			rr_name, 
			required_count, 
			time_to_manage, 
			rescuers,
			rescuer_types
		);

		// Incremento il contatore dei rescuer
		local_rescuer_req_number++;

		// Passo al token successivo
		token = strtok(NULL, ";");
	}

	*priority = local_priority;
	strcpy(emergency_desc, local_emergency_desc); 
	*rescuer_req_number = local_rescuer_req_number; // l'ultimo incremento lo ha reso il numero corretto
	// i nomi e le richieste sono già stati copiati nel buffer

}

int emergency_values_are_illegal(char *emergency_desc, short priority){
	return (
		strlen(emergency_desc) <= 0 || 
		priority < MIN_EMERGENCY_PRIORITY || 
		priority > MAX_EMERGENCY_PRIORITY
	);
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
	rescuer_type_t *rescuer_type = rescuer_request->type;
	return rescuer_type->rescuer_type_name;
}

void allocate_emergency_type(
	short priority, 
	char *emergency_desc, 
	int rescuer_req_number,
	emergency_type_t **emergency_types
){
	int i = 0;
	while(emergency_types[i] != NULL) i++;
	// allco l'emergency_type_t
	emergency_types[i] = (emergency_type_t *)malloc(sizeof(emergency_type_t));
	if(!emergency_types[i]) {
		perror("Errore allocazione memoria emergency_types");
		exit(EXIT_FAILURE);
	}
	// alloco l'array di rescuer_requests
	emergency_types[i]->rescuers = (rescuer_request_t **)calloc(rescuer_req_number + 1, sizeof(rescuer_request_t *));

}


allocate_rescuer_request(
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
	if(!rescuers[i]) {
		perror("Errore allocazione memoria rescuer_requests");
		exit(EXIT_FAILURE);
	}

	// popolo i campi
	rescuers[i]->type = get_rescuer_type_by_name(rr_name, rescuer_types);
	rescuers[i]->required_count = required_count;
	rescuers[i]->time_to_manage = time_to_manage;
}

