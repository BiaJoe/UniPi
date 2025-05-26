#include "client.h"

// client.c prende in input le emergenze e fa un minimo di error handling
// poi invia le emergenze, ma lascia al server il compito di finire il check dei valori



int main(int argc, char* argv[]){

	// controllo il numero di argomenti
	if(argc != 3 && argc != 5)
		DIE("numero di argomenti sbagliato");

	// capisco in quale modalità mi trovo
	// in questo caso le modalità sono 2 + undefined, ma non sarebbe difficile espandere il codice
	// ed aggiungerne di più in futuro

	int mode = UNDEFINED_MODE;

	if(argc == 5)
		mode = NORMAL_MODE;

	if(argc == 3){
		if(argv[1][0] != '-')
			DIE("opzione di lettura non specificata");
		switch(argv[1][1]){
			case 'f': mode = FILE_MODE; break;

			// ...espandibile per altre modalità
			default: DIE("opzione inesistente richiesta");
		}
	}

	switch (mode) {
		case NORMAL_MODE: handle_normal_mode_input(argv); break;
		case FILE_MODE: 	handle_file_mode_input(argv); break;
		default: 					DIE("modalità di inserimento non trovata");
	}

	// a questo punto l'emergenza o le emergenze sono state inviate.
	// sono tutte emergenze SINTATTICAMENTE valide
	// spetta al server controllare la SEMANTICA (se i valori x,y,timestamo sono nei limiti)
	// se non lo sono l'emergenza viene ignorata nel server

	return 0;
}



void handle_normal_mode_input(char* args[]){
	int x = 0;
	int y = 0;
	time_t delay = 0;
	char name[EMERGENCY_NAME_LENGTH + 1];
	int errors_occurred = 0;

	if(strlen(args[1]) > EMERGENCY_NAME_LENGTH){
		LOG_IGNORING_ERROR("nome emergenza troppo lungo");
		exit(EXIT_FAILURE);
	}

	x 		= my_atoi(args[2], &errors_occurred);
	y 		= my_atoi(args[3], &errors_occurred);
	delay = (time_t) my_atoi(args[4], &errors_occurred);

	if(errors_occurred){
		LOG_IGNORING_ERROR("caratteri non numerici presenti nei valori numerici dell'emergenza richiesta");
		exit(EXIT_FAILURE);
	}

	strcpy(name, args[1]); 
	emergency_request_t *e = mallocate_and_populate_emergency_request(name, x, y, delay);
	send_emergency_message(e);

	exit(EXIT_SUCCESS);
}

void handle_file_mode_input(char* args[]){
	FILE* f = fopen(args[2], "r");
	check_opened_file(f, args[2]);

	int x = 0;
	int y = 0;
	time_t delay = 0;
	char name[EMERGENCY_NAME_LENGTH + 1];
	char *line = NULL;
	size_t len = 0;
	int line_count = 0, emergency_count = 0;

	while (getline(&line, &len, f) != -1) {
		line_count++;
		if(line_count > MAX_FILE_LINES)
			log_fatal_error("linee massime superate nel client. Interruzione della lettura emergenze", FATAL_ERROR_PARSING);
		if(emergency_count > MAX_EMERCENCY_REQUEST_COUNT)
			log_fatal_error("linee massime superate nel client. Interruzione della lettura emergenze", FATAL_ERROR_PARSING);
		if(sscanf(line, EMERGENCY_REQUEST_SYNTAX, name, &x, &y, &delay) != 4){
			LOG_IGNORING_ERROR("sintassi della riga del file delle emergenze errata");
			continue; // ignoro la riga
		}
		emergency_request_t *e = mallocate_and_populate_emergency_request(name, x, y, delay);
		send_emergency_message(e);
		emergency_count++;
	}

	free(line);
	fclose(f);
}

void send_emergency_message(emergency_request_t* e){
 // todo
 printf("[%d, %d] %ld : %s ", e->x, e->y, e->timestamp, e->emergency_name);
}