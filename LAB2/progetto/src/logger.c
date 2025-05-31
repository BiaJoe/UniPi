#include "logger.h"

static FILE *log_file = NULL;

/*
Processo che riceve messaggi con message queue e li logga con log.c
*/

int main(void){

	//inizializza il loggging
	log_init(); 

	//riceve i messaggi
	mqd_t mq;
	char buffer[MAX_LOG_EVENT_LENGTH];

	check_error_mq_open(mq = mq_open(LOG_QUEUE_NAME, O_RDONLY));

	// ricevo i messaggi, se sono il messaggio di stop esco, altrimenti li scrivo nel logfile
	while (1) {
		check_error_mq_recieve(mq_receive(mq, buffer, MAX_LOG_EVENT_LENGTH, NULL) < 0);
		if(I_HAVE_TO_CLOSE_THE_LOG(buffer)) break;
		write_line(log_file, buffer);
	}

	mq_close(mq);
	mq_unlink(LOG_QUEUE_NAME);

	// Chiude il logging
	log_close(); 

	return 0;
}


// funzione che inizializza il logging, apre il file di log e scrive l'evento di inizio logging
void log_init() {
	check_error_NULL(log_file = fopen(LOG_FILE, "a"), "Errore apertura file di log");
	log_event(NO_ID, LOGGING_STARTED, "Inizio logging");
}

// funzione che chiude il logging, scrive l'evento di fine logging e chiude il file di log
void log_close() {
	log_event(NO_ID, LOGGING_ENDED, "Fine del logging");
	if (log_file) fclose(log_file);
}

