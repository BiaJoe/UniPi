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

	// si inizia a loggare
	log_event(NO_ID, LOGGING_STARTED, "Inizio logging");

	// estraggo le informazioni dai file conf, le metto tutte nel server context
	server_context_t *ctx = get_server_context();

	char buffer[MAX_EMERGENCY_QUEUE_MESSAGE_LENGTH];

	while (1) {
		check_error_mq_recieve(mq_receive(ctx->mq, buffer, MAX_EMERGENCY_QUEUE_MESSAGE_LENGTH, NULL));
		// COSA DEVO FARE????
	}

	// Libero la memoria, chiudo e unlinko la coda di messaggi
	cleanup_server_context(ctx);

	// si smette di loggare
	log_event(NO_ID, LOGGING_ENDED, "Fine del logging");

}

// faccio il parsing dei file
// ottengo numero e puntatore ad emergenze e rescuers
// creo la coda di messaggi ricevuti dal client
// inizializzo i mutex
server_context_t *get_server_context(){

	server_context_t *ctx = (server_context_t *)malloc(sizeof(server_context_t));

	// variabili inizializzate per il parsing
	int height = 0, width = 0;
	int rescuer_count = 0;
	int emergency_types_count = 0;
	rescuer_type_t** rescuer_types = NULL;
	emergency_type_t** emergency_types = NULL;

	// Parsing dei file di configurazione
	log_event(NO_ID, PARSING_STARTED, "Inizio parsing dei file di configurazione");

	parse_env(&height, &width);
	rescuer_types = parse_rescuers(&rescuer_count);
	emergency_types = parse_emergencies(&emergency_types_count, rescuer_types);

	log_event(NO_ID, PARSING_ENDED, "Il parsing è terminato con successo!");

	ctx -> height = height;
	ctx -> width = width;
	ctx -> rescuer_count = rescuer_count;
	ctx -> emergency_types_count = emergency_types_count;
	ctx -> emergency_requests_count = 0; // all'inizio non ci sono state ancora richieste
	ctx -> rescuer_types = rescuer_types;
  ctx -> emergency_types = emergency_types;

	struct mq_attr attr = {
		.mq_flags = 0,
		.mq_maxmsg = MAX_LOG_QUEUE_MESSAGES,
		.mq_msgsize = MAX_LOG_EVENT_LENGTH,
		.mq_curmsgs = 0
	};

	check_error_mq_open(ctx->mq = mq_open(EMERGENCY_QUEUE_NAME, O_CREAT | O_RDONLY, 0644, &attr));
}

// funzione semplice ma utile se in futuro vorrò cambiare il modo in cui sono immagazzinati i dati in ctx
emergency_type_t **get_emergency_types_from_context(server_context_t *ctx){
	return ctx -> emergency_types;
}

int get_server_height(server_context_t *ctx){
	return ctx -> height;
}
int get_server_width(server_context_t *ctx){
	return ctx -> width;
}

void cleanup_server_context(server_context_t *ctx){
	free_rescuer_types(ctx->rescuer_types);
	free_emergency_types(ctx->emergency_types);
	mq_close(ctx->mq);
	mq_unlink(EMERGENCY_QUEUE_NAME);
	free(ctx);
}