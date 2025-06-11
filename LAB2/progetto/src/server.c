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
	log_event(NO_ID, SERVER, "tutte le variabili sono state ottenute dal server: unizia il processo di listening di richieste");
	
	
	close_server(ctx);
}


int parse_emergency_request(char *message, char* name, int *x, int *y, time_t *timestamp){
	if(sscanf(message, EMERGENCY_REQUEST_SYNTAX, name, x, y, timestamp) != 4)
		return 0;
	return 1;
}

int emergency_request_values_are_illegal(server_context_t *ctx, char* name, int x, int y, time_t timestamp){
	if(strlen(name) <= 0) return YES;
	int h = get_server_height(ctx);
	int w = get_server_width(ctx);
	if(!get_emergency_type_by_name(name, get_emergency_types_from_server_context(ctx))) return YES;
	if(ABS(x) < MIN_X_COORDINATE_ABSOLUTE_VALUE || ABS(x) > ABS(w)) return YES;
	if(ABS(y) < MIN_Y_COORDINATE_ABSOLUTE_VALUE || ABS(y) > ABS(h)) return YES;
	if(timestamp == INVALID_TIME) return YES;
	return NO;
}

// thread function che riceve le emergenze
// le scarta se sbagliate
// le inserisce nella queue

int recieve_emergency_requests(server_context_t *ctx){
	log_event(NO_ID, MESSAGE_QUEUE_SERVER, "inizio della ricezione delle emergenze!");
	char buffer[MAX_EMERGENCY_QUEUE_MESSAGE_LENGTH];

	while (1) {
		check_error_mq_recieve(mq_receive(ctx->mq, buffer, MAX_EMERGENCY_QUEUE_MESSAGE_LENGTH, NULL));
		if(IS_STOP_MESSAGE(buffer)) close_server(ctx);
		char *name; int x, y, this_is_to_ignore = 0; time_t time;
		if(!parse_emergency_request(buffer, name, &x, &y, &time)) this_is_to_ignore = 1;
		if(emergency_request_values_are_illegal(ctx, name, x, y, time)) this_is_to_ignore = 1;
		if(this_is_to_ignore){ 
			LOG_IGNORE_EMERGENCY_REQUEST("emergenza rifiutata perchè conteneva valori illegali");
			continue;
		}
		emergency_t *e = mallocate_emergency(ctx, name, x, y, time);
		emergency_queue_t *q = get_emergency_queue_from_context(ctx);

		lock_queue(q);
			enqueue_emergency(q, e);
		unlock_queue(q);

		LOG_EMERGENCY_REQUEST_RECIVED();
	}

}

void close_server(server_context_t *ctx){
	log_event(NO_ID, SERVER, "lavoro finito. Il server si avvia alla chiusura.");
	log_event(NO_ID, LOGGING_ENDED, "Fine del logging");
	cleanup_server_context(ctx);
	exit(EXIT_SUCCESS);
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

	// popolo ctx
	ctx -> height = height;
	ctx -> width = width;
	ctx -> rescuer_count = rescuer_count;
	ctx -> emergency_types_count = emergency_types_count;
	ctx -> emergency_requests_count = 0; // all'inizio non ci sono state ancora richieste
	ctx -> rescuer_types = rescuer_types;
  ctx -> emergency_types = emergency_types;
	ctx -> queue = mallocate_emergency_queue();
	struct mq_attr attr = {
		.mq_flags = 0,
		.mq_maxmsg = MAX_LOG_QUEUE_MESSAGES,
		.mq_msgsize = MAX_LOG_EVENT_LENGTH,
		.mq_curmsgs = 0
	};
	check_error_mq_open(ctx->mq = mq_open(EMERGENCY_QUEUE_NAME, O_CREAT | O_RDONLY, 0644, &attr));
}

emergency_type_t **get_emergency_types_from_context(server_context_t *ctx){
	return ctx -> emergency_types;
}

rescuer_type_t **get_rescuer_types_from_context(server_context_t *ctx){
	return ctx -> rescuer_types;
}

emergency_queue_t *get_emergency_queue_from_context(server_context_t *ctx){
	return ctx -> queue;
}

void lock_queue(emergency_queue_t *q){
	LOCK(q->queue_mutex);
}

void unlock_queue(emergency_queue_t *q){
	UNLOCK(q->queue_mutex);
}

int get_server_height(server_context_t *ctx){
	return ctx -> height;
}

int get_server_width(server_context_t *ctx){
	return ctx -> width;
}

void cleanup_server_context(server_context_t *ctx){
	free_rescuer_types(get_rescuer_types_from_context(ctx));
	free_emergency_types(get_emergency_types_from_context(ctx));
	free_emergency_queue(get_emergency_queue_from_context(ctx));
	mq_close(ctx->mq);
	mq_unlink(EMERGENCY_QUEUE_NAME);
	free(ctx);
}