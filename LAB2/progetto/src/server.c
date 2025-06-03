#include "server.h"

int height;
int width;

int main(void){

	// divido in due processi
	// mentre il logger aspetta per messaggi da loggare
	// il server fa il parsing e aspetta per richieste di emergenza
	FORK_PROCESS(logger, server);

	return 0;
}

void server(void){
	// variabili inizializzate per il parsing
	int rescuer_count = 0;
	int emergency_count = 0;
	rescuer_type_t** rescuers = NULL;
	emergency_type_t** emergency_types = NULL;

	// si inizia a loggare
	log_event(NO_ID, LOGGING_STARTED, "Inizio logging");

	// Parsing dei file di configurazione
	log_event(NO_ID, PARSING_STARTED, "Inizio parsing dei file di configurazione");

	parse_env(&height, &width);
	rescuers = parse_rescuers(&rescuer_count);
	emergency_types = parse_emergencies(&emergency_count, rescuers);

	log_event(NO_ID, PARSING_ENDED, "Il parsing è terminato con successo!");

	// Stampa delle informazioni
	// print_env(queue, height, width);
	// print_rescuer_types(rescuers, rescuer_count);
	// print_emergency_types(emergency_types, emergency_count);

	// ASPETTO I MESSAGGI DAL CLIENT
	mqd_t mq;
	struct mq_attr attr;
	char buffer[MAX_LOG_EVENT_LENGTH];

	attr.mq_flags = 0;
	attr.mq_maxmsg = MAX_LOG_QUEUE_MESSAGES;
	attr.mq_msgsize = MAX_LOG_EVENT_LENGTH;
	attr.mq_curmsgs = 0;

	check_error_mq_open(mq = mq_open(EMERGENCY_QUEUE_NAME, O_CREAT | O_RDONLY, 0644, &attr));

	// ricevo i messaggi
	// se sono il messaggio di stop esco perché per qualche motivo ho finito
	// altrimenti li scrivo nel logfile
	while (1) {
		check_error_mq_recieve(mq_receive(mq, buffer, MAX_LOG_EVENT_LENGTH, NULL));

	}

	// Libero la memoria
	free_rescuer_types(rescuers);
	free_emergency_types(emergency_types);

	// si smette di loggare
	log_event(NO_ID, LOGGING_ENDED, "Fine del logging");

}