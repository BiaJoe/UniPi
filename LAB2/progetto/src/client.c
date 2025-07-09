#include "client.h"

// client.c prende in input le emergenze e fa un minimo di error handling
// poi invia le emergenze, ma lascia al server il compito di finire il check dei valori

// coda delle emergenze
mqd_t mq;

int main(int argc, char* argv[]){
	// controllo il numero di argomenti
	if(argc != 3 && argc != 5) DIE(argv[0], "numero di argomenti dati al programma sbagliato");

	// capisco in quale modalità mi trovo
	// in questo caso le modalità sono 2 + undefined, 
	// ma non sarebbe difficile espandere il codice ed aggiungerne di più in futuro
	int mode = UNDEFINED_MODE;

	if(argc == 5)
		mode = NORMAL_MODE;

	if(argv[1][0] == '-')
		switch(argv[1][1]) {
			case FILE_MODE_CHAR: mode = FILE_MODE; break;
			case STOP_MODE_CHAR: mode = STOP_MODE; break;
			// ...espandibile ad altre modalità del tipo ./client -<mode> <arg>
			default: DIE(argv[0], "opzione inesistente richiesta");
		}

	// apro la coda su cui manderò la/le emergenza/e
	check_error_mq_open(mq = mq_open(EMERGENCY_QUEUE_NAME, O_WRONLY));

	switch (mode) {
		case NORMAL_MODE: handle_normal_mode_input(argv); break;
		case FILE_MODE: 	handle_file_mode_input(argv); break;
		case STOP_MODE: 	handle_stop_mode_client(); break;
		default: 					DIE(argv[0], "modalità di inserimento non trovata");
	}

	// a questo punto l'emergenza o le emergenze sono state inviate.
	// sono tutte emergenze SINTATTICAMENTE valide
	// spetta al server controllare la SEMANTICA (se i valori x,y,timestamo sono nei limiti)
	// se non lo sono l'emergenza viene ignorata nel server
	log_event(AUTOMATIC_LOG_ID, MESSAGE_QUEUE_CLIENT, "il lavoro del client è finito. Il processo si chiude");

	return 0;
}


void handle_stop_mode_client(void){
	char *buffer = STOP_MESSAGE_FROM_CLIENT;
	check_error_mq_send(mq_send(mq, buffer, strlen(buffer) + 1, 0));
	log_event(AUTOMATIC_LOG_ID, MESSAGE_QUEUE_CLIENT, "inviato messaggio di stop dal client");
}

// prende nome, coordinate e timestamp in forma di stringhe, fa qualche controllo e li spedisce
// ritorna 0 se fallisce senza spedire, 1 se riesce
int send_emergency_request_message(char *name, char *x_string, char *y_string, char *delay_string) {
	errno = 0; // my_atoi usa errno
	int x = my_atoi(x_string);
	int y = my_atoi(y_string);
	int d = my_atoi(delay_string);
	d = ABS(d); // per sicurezza faccio il valore assoluto

	char buffer[MAX_EMERGENCY_REQUEST_LENGTH + 1];

	if(errno != 0){
		LOG_IGNORING_ERROR("caratteri non numerici presenti nei valori numerici dell'emergenza richiesta");
		return 0;
	}

	if(strlen(name) > EMERGENCY_NAME_LENGTH){
		LOG_IGNORING_ERROR("nome emergenza troppo lungo");
		return 0;
	}

	if(snprintf(buffer, sizeof(buffer), "%s %d %d %ld", name, x, y, d) >= MAX_EMERGENCY_REQUEST_LENGTH + 1){
		LOG_IGNORING_ERROR("messaggio di emergenza troppo lungo");
		return 0;
	}

	// aspetto il delay prima di inviare l'emergenza, in modo da poterla subito processare nel server senza dover aspettare
	sleep(d);

	// il client garantisce la correttezza sintattica della richiesta
	// al server spetta controllare la correttezza semantica e processare la richiesta
	check_error_mq_send(mq_send(mq, buffer, strlen(buffer) + 1, 0));
	log_event(AUTOMATIC_LOG_ID, MESSAGE_QUEUE_CLIENT, "inviata emergenza al server");

	return 1;
}

// gestisce la singola emergenza passata da terminale
void handle_normal_mode_input(char* args[]){
	log_event(AUTOMATIC_LOG_ID, MESSAGE_QUEUE_CLIENT, "avvio della modalità di inserimento diretta");
	send_emergency_request_message(args[1], args[2], args[3], args[4]);
}

// gestisce l'emergenza passata per file inviando un'emergenza alla volta riga per riga    
void handle_file_mode_input(char* args[]){
	log_event(AUTOMATIC_LOG_ID, MESSAGE_QUEUE_CLIENT, "avvio della modalità di inserimento da file");
	FILE* emergency_requests_file = fopen(args[2], "r");
	check_error_fopen(emergency_requests_file);

	char *name, *x, *y, *d, *must_be_null;

	char *line = NULL;
	size_t len = 0;
	int line_count = 0, emergency_count = 0;

	while (getline(&line, &len, emergency_requests_file) != -1) {
		line_count++;

		if(line_count > MAX_FILE_LINES)
			log_fatal_error_temporaneo("linee massime superate nel client. Interruzione della lettura emergenze", FATAL_ERROR_PARSING);
		if(emergency_count > MAX_EMERGENCY_REQUEST_COUNT)
			log_fatal_error_temporaneo("numero di emergenze richieste massime superate nel client. Interruzione della lettura emergenze", FATAL_ERROR_PARSING);
		
		name 					= strtok(line, EMERGENCY_REQUEST_ARGUMENT_SEPARATOR);
		x 						= strtok(NULL, EMERGENCY_REQUEST_ARGUMENT_SEPARATOR);
		y 						= strtok(NULL, EMERGENCY_REQUEST_ARGUMENT_SEPARATOR);
		d 						= strtok(NULL, EMERGENCY_REQUEST_ARGUMENT_SEPARATOR);
		must_be_null 	= strtok(NULL, EMERGENCY_REQUEST_ARGUMENT_SEPARATOR);

		if(name == NULL || x == NULL || y == NULL || d == NULL || must_be_null != NULL){
			LOG_IGNORING_ERROR("riga del file di emergenze sbagliata");
			continue;
		}
		
		if(!send_emergency_request_message(name, x, y, d))
			continue;

		emergency_count++;
	}

	free(line);
	fclose(emergency_requests_file);

	log_event(AUTOMATIC_LOG_ID, MESSAGE_QUEUE_CLIENT, "Invio di emergency_request da file %s completato: %d emergenze lette, %d inviate", args[2], line_count, emergency_count);
}

