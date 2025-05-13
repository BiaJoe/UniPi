#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <ctype.h>
#include "../include/parsers.h"

emergency_type_t** parse_emergencies(int *emergency_count){
	// Apro il file di configurazione
	FILE *emergencies_conf = fopen(EMERGENCY_TYPES_CONF, "r");
	if(!emergencies_conf) { 
		perror("Errore apertura file di configurazione " EMERGENCY_TYPES_CONF);
		exit(EXIT_FAILURE);
	}

	char *line = NULL;
	size_t len = 0;
	char emergency_desc[EMERGENCY_NAME_LENGTH]; // buffer per il nome dell'emergenza
	int resquers_req_number;		// variabili temporanee per i campi dell'emergenza
	short priority;

	//buffer per i nomi delle emergenze inizializzato a'\0'
	char emergency_names_buffer[MAX_FILE_LINES][EMERGENCY_NAME_LENGTH]; 
	for (int i = 0; i < MAX_FILE_LINES; i++) emergency_names_buffer[i][0] = '\0'; 
	
	// buffer per i tipi di emergenza inizializzato a 0
	emergency_type_t emergency_types_buffer[MAX_FILE_LINES];
	for(int i = 0; i<MAX_FILE_LINES; i++){
		emergency_types_buffer[i].priority = 0;
		emergency_types_buffer[i].emergency_desc = NULL;
		emergency_types_buffer[i].rescuers = NULL;
		emergency_types_buffer[i].rescuers_req_number = 0;
	} 

	// buffer per i nomi dei rescuer necessari inizializzato a'\0'
	char rescuer_names_buffer[MAX_RESCUER_REQ_NUMBER_PER_EMERGENCY][MAX_RESCUER_NAME_LENGTH];
	for (int i = 0; i < MAX_FILE_LINES; i++) rescuer_names_buffer[i][0] = '\0';

	// buffer per rescuer_request_t inizializzato a 0
	rescuer_request_t rescuer_requests_buffer[MAX_RESCUER_REQ_NUMBER_PER_EMERGENCY];
	for(int i = 0; i<MAX_RESCUER_REQ_NUMBER_PER_EMERGENCY; i++){
		rescuer_requests_buffer[i].type = NULL;
		rescuer_requests_buffer[i].required_count = 0;
		rescuer_requests_buffer[i].time_to_manage = 0;
	}

	// contatori di linee e numero di emergenze
	int local_emergency_count = 0;
	int line_count = 0;

	// Leggo ogni riga del file e processo le informazioni contenute
	while (getline(&line, &len, emergencies_conf) != -1) {
		line_count++;

		// Controllo che il numero massimo di linee non venga superato
		if(line_count > MAX_FILE_LINES){
			perror("Numero massimo di linee superato file di configurazione " EMERGENCY_TYPES_CONF);
			exit(EXIT_FAILURE);
		}

		// Ignoro le righe vuote
		if (line[0] == '\n') continue;

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
			rescuer_names_buffer,
			rescuer_requests_buffer, 
			&resquers_req_number,
			emergency_names_buffer
		);

		local_emergency_count++;
		
		// adesso i valori sono stati estratti e sono validi
		

	}

}


// si occupa di controllare la sintassi e di estrarre i valori di UNA riga
void check_emergency_type_syntax_and_extract_values(
	char *line, 
	short *priority, 
	char *emergency_desc, 
	char rescuer_names_buffer[][MAX_RESCUER_NAME_LENGTH],
	rescuer_request_t *rescuer_requests_buffer, 
	int *rescuer_req_number,
	char emergency_names_buffer[][EMERGENCY_NAME_LENGTH]
){
	char rr_string[MAX_LINE_LENGTH]; // buffer per la stringa dei rescuer
	int local_rescuer_req_number = 0; // contatore dei rescuer
	char local_emergency_desc[EMERGENCY_NAME_LENGTH]; // buffer per il nome dell'emergenza
	int local_priority; // buffer per la priorità

	// estraggo il nome dell'emergenza, la priorità e la stringa dei rescuer
	if(sscanf(line, "[%49[^]]] [%d] %[^\n]", local_emergency_desc, &local_priority, rr_string) !=3){
		perror("Errore di sintassi file di configurazione " EMERGENCY_TYPES_CONF);
		exit(EXIT_FAILURE);
	}

	// Controllo che i valori siano validi
	if(
		strlen(local_emergency_desc) <= 0 || 
		local_priority < MIN_PRIORITY || 
		local_priority > MAX_PRIORITY
	){
		perror("Valori illegali file di configurazione " EMERGENCY_TYPES_CONF);
		exit(EXIT_FAILURE);
	}

	// Controllo se il nome dell'emergenza è già presente nel buffer dei nomi, altrimenti lo aggiungo
	if(emergency_arleady_exists(emergency_desc, emergency_names_buffer)){
		perror("Nome emergenza già presente nel file di configurazione " EMERGENCY_TYPES_CONF);
		exit(EXIT_FAILURE);
	}

	// tokenizzo la stringa dei rescuer
	char *token = strtok(rr_string, ";");
	char rr_name[MAX_RESCUER_NAME_LENGTH]; // buffer per il singolo nome del rescuer
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
				"%49[^:]:%d:%d", 
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

		// Controllo se il nome del rescuer è già presente nel buffer. Se non lo è, lo aggiungo
		if(rescuer_arleady_exists(rr_name, rescuer_names_buffer)){
			perror("Nome rescuer già presente nella lista di rescuer richiesti per l'emergenza " EMERGENCY_TYPES_CONF);
			exit(EXIT_FAILURE);
		}

		// Aggiungo il nome del rescuer al buffer
		strcpy(rescuer_names_buffer[local_rescuer_req_number], rr_name);

		// Aggiungo la rescuer_request al buffer
		rescuer_requests_buffer[local_rescuer_req_number].required_count = required_count;
		rescuer_requests_buffer[local_rescuer_req_number].time_to_manage = time_to_manage;

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

int emergency_arleady_exists(char *name, char emercency_names_buffer[][EMERGENCY_NAME_LENGTH]){
	int last_index = 0;

	for(int i = 0; i < MAX_FILE_LINES; i++){

		// se siamo al primo posto libero (abbiamo finito i nomi da controllare)
		if(emercency_names_buffer[i][0] == '\0'){ 
			last_index = i;	
			break;
		}

		// Se il nome è già presente
		if(strcmp(name, emercency_names_buffer[i]) == 0){ 
			return 1;
		}
	}

	// Se non è presente lo aggiungo alla fine del buffer
	strcpy(emercency_names_buffer[last_index], name);
	return 0;
}