#include "parsers.h"

static int rescuer_requests_total_count = 0;

emergency_type_t** parse_emergencies(int *emergency_count, rescuer_type_t **rescuer_types){
	// Apro il file di configurazione
	FILE *emergencies_conf = fopen(EMERGENCY_TYPES_CONF, "r");
	check_opened_file_error_log(emergencies_conf);

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
		if(local_emergency_count > MAX_EMERGENCY_TYPES_COUNT)
			log_fatal_error("Numero massimo di emergenze superato nel file di configurazione: " EMERGENCY_TYPES_CONF, FATAL_ERROR_PARSING);

		// Controllo che il numero massimo di linee non venga superato
		if(line_count > MAX_FILE_LINES)
			log_fatal_error("Numero massimo di linee superato file di configurazione " EMERGENCY_TYPES_CONF, FATAL_ERROR_PARSING);

		// Controllo la lunghezza della riga
		if(strlen(line) > MAX_LINE_LENGTH)
			log_fatal_error("Riga troppo lunga nel file di configurazione: " EMERGENCY_TYPES_CONF, FATAL_ERROR_PARSING);

		//ignoro le righe vuote
		if (line[0] == '\n') {
			log_event(line_count, EMPTY_CONF_LINE_IGNORED, "Riga vuota ignorata: " EMERGENCY_TYPES_CONF);
			continue;
		}

		// inizializzo dinamicamente l'array di rescuer di questa emergenza 
		// sarà ridichiarato e inizializzato ad ogni ciclo e assegnato all'emergenza
		rescuer_request_t **rescuers = callocate_resquer_requests();

		// Controllo che la sintassi sia corretta ed estraggo i valori
		check_emergency_type_syntax_and_extract_values(
			line_count,
			line, 
			&priority, 
			emergency_desc, 
			rescuers,
			&resquers_req_number,
			rescuer_types
		);

		if(get_emergency_type_by_name(emergency_desc, emergency_types)){
			log_event(line_count, DUPLICATE_EMERGENCY_TYPE_IGNORED, "Nome emergenza già presente, non aggiunta: " EMERGENCY_TYPES_CONF);
			free_rescuer_requests(rescuers); // libero la memoria allocata per i rescuer, che non mi serve per questa riga
			continue; // ignoro la riga
		}

		mallocate_and_populate_emergency_type(
			priority,
			emergency_desc, 
			resquers_req_number,
			rescuers,
			emergency_types
		);

		local_emergency_count++;
		log_event(local_emergency_count, EMERGENCY_PARSED, "emergenza letta: " EMERGENCY_TYPES_CONF);
	}

	*emergency_count = local_emergency_count; // restituisco il numero di emergenze lette
	return emergency_types;	
}





// si occupa di controllare la sintassi e di estrarre i valori di UNA riga
void check_emergency_type_syntax_and_extract_values(
	int line_count,
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
	if(sscanf(line, EMERGENCY_TYPE_SYNTAX, local_emergency_desc, &local_priority, rr_string) !=3)
		log_fatal_error("Errore di sintassi nel file di configurazione " EMERGENCY_TYPES_CONF, FATAL_ERROR_PARSING);

	// Controllo che i valori strettamente legati all'emergenza siano validi
	if(emergency_values_are_illegal(local_emergency_desc, local_priority))
		log_fatal_error("Valori illegali nel file di configurazione: " EMERGENCY_TYPES_CONF, FATAL_ERROR_PARSING);

	// tokenizzo la stringa dei rescuer
	char *token = strtok(rr_string, ";");
	char rr_name[MAX_RESCUER_NAME_LENGTH]; // buffer per il nome del rescuer
	int required_count, time_to_manage; // variabili temporanee per i campi del rescuer

	while(token != NULL){
		
		//Controllo che il numero massimo di rescuer non venga superato
		if(local_rescuer_req_number > MAX_RESCUER_REQ_NUMBER_PER_EMERGENCY) // magg. stretto perché il contatore parte da 0
			log_fatal_error("Numero massimo di rescuer richiesti per una sola emergenza superato: " EMERGENCY_TYPES_CONF, FATAL_ERROR_PARSING);

		// Controllo la sintassi del rescuer
		if(
			sscanf(
				token, 
				RESCUER_REQUEST_SYNTAX, 
				rr_name, 
				&required_count, 
				&time_to_manage
			) != 3
		) log_fatal_error("Errore di sintassi nel file di configurazione " EMERGENCY_TYPES_CONF, FATAL_ERROR_PARSING);

		// Controllo che i valori siano validi
		if(rescuer_request_values_are_illegal(rr_name, required_count, time_to_manage))
			log_fatal_error("Valori illegali nel file di configurazione: " EMERGENCY_TYPES_CONF, FATAL_ERROR_PARSING);


		// Controllo se il rescuer è già presente. Se non lo è, lo aggiungo
		if(get_rescuer_request_by_name(rr_name, rescuers)){
			log_event(line_count, DUPLICATE_RESCUER_REQUEST_IGNORED, "Nome rescuer richiesto più di una volta da una singola emergenza, non aggiunto: " EMERGENCY_TYPES_CONF);
			token = strtok(NULL, ";");
			continue; // ignoro questo token
		}

		// Controllo che il rescuer_type sia valido (cioè che esista in rescuer_types)
		if(!get_rescuer_type_by_name(rr_name, rescuer_types))
			log_fatal_error("Richiesto rescuer inesistente: " EMERGENCY_TYPES_CONF, FATAL_ERROR_PARSING);

		// Il rescuer richiesto è valido, quindi lo alloco e aggiungo alla lista di rescuer richiesti per l'emergenza
		mallocate_and_populate_rescuer_request(
			rr_name, 
			required_count, 
			time_to_manage, 
			rescuers,
			rescuer_types
		);

		// Incremento il contatore dei rescuer
		local_rescuer_req_number++;
		rescuer_requests_total_count++; // incremento il contatore totale dei rescuer richiesti
		log_event(rescuer_requests_total_count, RESCUER_REQUEST_ADDED, "rescuer richiesto aggiunto: " EMERGENCY_TYPES_CONF);

		// Passo al token successivo
		token = strtok(NULL, ";");
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

