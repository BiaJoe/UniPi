#include "server.h"

// ----------- funzioni per il thread clock e chi deve avere accesso ai tick del server ----------- 


// thread funztion per il clock: manda un messaggio di tick ad ogni tick del server
void thread_clock(void *arg){
	server_context_t *ctx = arg;
	while(1){
		thrd_sleep(SERVER_TICK_TIME, NULL); 									// attendo un tick di tempo del server
		lock_server_clock(ctx);																// blocco il mutex per il tempo del server
		tick(ctx); 																		 				// il sterver ha tickato
		cnd_broadcast(&ctx->clock_updated);								 			// segnalo al thread updater che il tempo è stato aggiornato
		unlock_server_clock(ctx);
	}
}

void tick(server_context_t *ctx){
	ctx->tick = YES; 						
	ctx->tick_count_since_start++; 	// incremento il contatore dei tick del server perchè è appena avvenuto un tick														
}

void untick(server_context_t *ctx){
	ctx->tick = NO; 																				
}

void wait_for_a_tick(server_context_t *ctx){
	cnd_wait(&ctx->clock_updated, &ctx->clock_mutex); 	// attendo che il thread clock mi segnali che il tempo è stato aggiornato
}

int server_is_ticking(server_context_t *ctx){
	return ctx->tick;
}
