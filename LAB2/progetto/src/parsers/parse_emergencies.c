#include "utils.h"

emergency_type_t** parse_emergencies(int *emergency_count, rescuer_type_t **rescuer_types){
	// Apro il file di configurazione
	FILE *emergencies_conf = fopen(EMERGENCY_TYPES_CONF, "r");
	if(!emergencies_conf) { 
		perror("Errore apertura file di configurazione " EMERGENCY_TYPES_CONF);
		exit(EXIT_FAILURE);
	}

	// inizializzo l'array di emergenze dinamicamente a NULL
	emergency_type_t ** emergency_types = callocate_emergency_types();

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

		// Controllo che il numero massimo di emergenze non venga superato
		if(local_emergency_count > MAX_EMERGENCY_TYPES_COUNT){
			perror("Numero massimo di emergenze superato nel file di configurazione " EMERGENCY_TYPES_CONF);
			exit(EXIT_FAILURE);
		}

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

		// inizializzo dinamicamente l'array di rescuer di questa emergenza 
		// sarà ridichiarato e inizializzato ad ogni ciclo e assegnato all'emergenza
		rescuer_request_t **rescuers = callocate_resquer_requests();

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

		mallocate_emergency_type(
			priority,
			emergency_desc, 
			resquers_req_number,
			rescuers,
			emergency_types
		);

		local_emergency_count++;
	}

	*emergency_count = local_emergency_count; // restituisco il numero di emergenze lette
	return emergency_types;	
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
	char rr_string[MAX_RESCUER_REQUESTS_LENGTH]; // buffer per la stringa dei rescuer
	int local_rescuer_req_number = 0; // contatore dei rescuer
	char local_emergency_desc[EMERGENCY_NAME_LENGTH]; // buffer per il nome dell'emergenza
	int local_priority = MIN_EMERGENCY_PRIORITY; 

	// estraggo il nome dell'emergenza, la priorità e la stringa dei rescuer
	if(sscanf(line, EMERGENCY_TYPE_SYNTAX, local_emergency_desc, &local_priority, rr_string) !=3){
		perror("Errore di sintassi file di configurazione " EMERGENCY_TYPES_CONF);
		printf("line: %s\n", line);
		printf("local_emergency_desc: %s\n", local_emergency_desc);
		printf("local_priority: %d\n", local_priority);
		printf("rr_string: %s\n", rr_string);
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
				&required_count, 
				&time_to_manage
			) != 3
		){
			perror("Errore di sintassi file di configurazione " EMERGENCY_TYPES_CONF);
			exit(EXIT_FAILURE);
		}

		// Controllo che i valori siano validi
		if(rescuer_request_values_are_illegal(rr_name, required_count, time_to_manage)){
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
		mallocate_rescuer_request(
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
}

int emergency_values_are_illegal(char *emergency_desc, short priority){
	return (
		strlen(emergency_desc) <= 0 || 
		priority < MIN_EMERGENCY_PRIORITY || 
		priority > MAX_EMERGENCY_PRIORITY
	);
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



