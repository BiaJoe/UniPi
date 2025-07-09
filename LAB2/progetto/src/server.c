#include "server.h"

// divido in due processi: logger e server
// mentre il logger aspetta per messaggi da loggare
// il server fa il parsing e aspetta per richieste di emergenza
int main(void){
	FORK_PROCESS(logger, server);
	return 0;
}

void server(void){
	
	log_event(NON_APPLICABLE_LOG_ID, LOGGING_STARTED, "Inizio logging");		// si inizia a loggare
	server_context_t *ctx = mallocate_server_context();											// estraggo le informazioni dai file conf, le metto tutte nel server context
	log_event(NON_APPLICABLE_LOG_ID, SERVER, "tutte le variabili sono state ottenute dal server: adesso il sistema è a regime!");
	
	thrd_t clock_thread;
	thrd_t updater_thread;
	thrd_t receiver_thread;

	if (
		thrd_create(&clock_thread, thread_clock, ctx) != thrd_success ||
		thrd_create(&updater_thread, thread_updater, ctx) != thrd_success
	)	{
		log_fatal_error("errore durante la creazione di thread nel server");
		return;
	}

	thrd_t worker_threads[THREAD_POOL_SIZE];
	for (int i = 0; i < THREAD_POOL_SIZE; i++) {
		if (thrd_create(&worker_threads[i], thread_worker_function, ctx) != thrd_success) {
			log_fatal_error("errore durante la creazione di thread nel server");
			return;
		}
	}

	thread_reciever(ctx);

	close_server(ctx);
}






void close_server(server_context_t *ctx){
	log_event(AUTOMATIC_LOG_ID, SERVER, "lavoro finito. Il server si avvia alla chiusura.");
	log_event(AUTOMATIC_LOG_ID, LOGGING_ENDED, "Fine del logging");
	cleanup_server_context(ctx);
	exit(EXIT_SUCCESS);
}


