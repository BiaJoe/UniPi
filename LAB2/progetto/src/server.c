#include "server.h"
int main(void){

	// divido in due processi
	// mentre il logger aspetta per messaggi da loggare
	// il server fa il parsing e aspetta per richieste di emergenza

	FORK_PROCESS(logger, server);

	return 0;
}

void server(void){
	
	log_event(NO_ID, LOGGING_STARTED, "Inizio logging");		// si inizia a loggare
	server_context_t *ctx = get_server_context();						// estraggo le informazioni dai file conf, le metto tutte nel server context
	log_event(NO_ID, SERVER, "tutte le variabili sono state ottenute dal server: adesso il sistema è a regime!");
	
	// faccio partire i thread 

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

void trhead_receiver(server_context_t *ctx){
	log_event(NO_ID, MESSAGE_QUEUE_SERVER, "inizio della ricezione delle emergenze!");
	char buffer[MAX_EMERGENCY_QUEUE_MESSAGE_LENGTH];

	while (1) {
		check_error_mq_recieve(mq_receive(ctx->mq, buffer, MAX_EMERGENCY_QUEUE_MESSAGE_LENGTH, NULL));
		if(IS_STOP_MESSAGE(buffer)) close_server(ctx);
		char *name; int x, y; time_t time;
		if(!parse_emergency_request(buffer, name, &x, &y, &time) || emergency_request_values_are_illegal(ctx, name, x, y, time)){ 
			LOG_IGNORE_EMERGENCY_REQUEST("emergenza rifiutata perchè conteneva valori illegali");
			continue;
		}
		emergency_t *e = mallocate_emergency(ctx, name, x, y, time);
		emergency_queue_t *q = get_emergency_queue_from_context(ctx);

		lock_queue(q);
			enqueue_emergency(q, e);
		unlock_queue(q);

		log_event(NO_ID, EMERGENCY_REQUEST_RECEIVED, "emergenza ricevuta e messa in attesa di essere processata!");
	}
}

// thread funztion per il clock

void thread_clock(void *arg){
	server_context_t *ctx = arg;
	while(1){
		thrd_sleep(SERVER_TICK_TIME, NULL); 									 	// attendo un tick del server
		lock_server_clock(ctx);																 	// blocco il mutex per il tempo del server
			set_server_current_time(ctx, time(NULL)); 	 					// incremento il tempo del server di un secondo
			log_event(NO_ID, SERVER, "il tempo del server è stato aggiornato a %ld", get_server_current_time(ctx)); // loggo l'evento
			tick(ctx); 																		 				// il sterver ha tickato
			increment_ticks(ctx);  										 						// incremento il contatore dei tick del server
			cnd_signal(&ctx->clock_updated);								 			// segnalo al thread updater che il tempo è stato aggiornato
		unlock_server_clock(ctx);
	}
}

// funzioni helper per il thread updater

void update_rescuers_positions_on_the_map_blocking(server_context_t *ctx){
	int amount = get_server_rescuer_types_count(ctx);
	lock_rescuer_types(ctx); 																		// blocco il mutex per i rescuer types
		for(int i = 0; i < amount; i++){													// aggiorno le posizioni dei gemelli rescuers
			rescuer_type_t *r = get_rescuer_type_by_index(ctx, i);
			if(r == NULL) continue; 																// se il rescuer type è NULL non faccio nulla (precauzione)
			for(int j = 0; j < get_rescuer_type_amount(r); j++){
				rescuer_digital_twin_t *dt = get_rescuer_digital_twin_by_index(r, j);
				update_rescuer_digital_twin_position(dt);
			}
		}
	unlock_rescuer_types(ctx);
}

void promote_or_timeout_expired_emergencies_in_queue_blocking(server_context_t *ctx){
	emergency_queue_t *q = get_emergency_queue_from_context(ctx);
	lock_queue(q);
		emergency_node_t *n = ctx->queue->lists[MIN_EMERGENCY_PRIORITY]->head;
		emergency_node_t *m = n->next;

		// promuovo le emergenze da min priority a medium priority se sono in coda da troppo tempo
		while(m != NULL){																									
			if(promote_to_medium_priority_if_needed(ctx->queue, n)) {				
				log_event(NO_ID, EMERGENCY_STATUS, "emergenza promossa da priorità minima alla priorità superiore perchè in coda da troppo tempo");
				n = m;
				if(m) m = m->next; 	// se non siamo all'ultimo nodo (se ci siamo alla prossima guardia del while si esce)
				continue; 
			}		
			n = n->next;
			m = n->next;
		}
		
		// metto in timeout le emergenze che hanno aspettato troppo
		for(int i = MEDIUM_EMERGENCY_PRIORITY; i <= MAX_EMERGENCY_PRIORITY; i++){
			emergency_node_t *n = ctx->queue->lists[i]->head;
			while(n != NULL){
				if(timeout_emergency_if_needed(ctx->queue, n))
					log_event(NO_ID, EMERGENCY_STATUS, "emergenza messa in timeout perchè ha aspettato troppo tempo");
				n = n->next;
			}
		}
		
	unlock_queue(q);

}


void thread_updater(void *arg){
	server_context_t *ctx = arg;
	while(1){
		lock_server_clock(ctx); 																					
		while(!server_is_ticking(ctx))																		// attendo che il server ticki
			wait_for_a_tick(ctx); 									
		

		unlock_server_clock(ctx); 																				
		log_event(NO_ID, SERVER, "inizio aggiornamento del server...");
		
		update_rescuers_positions_on_the_map_blocking(ctx); 							// aggiorno le posizioni dei rescuers sulla mappa
		promote_or_timeout_expired_emergencies_in_queue_blocking(ctx); 		// promuovo da min priority a medium priority oppure metto in timeout le emergenze che hanno aspettato troppo
		
		log_event(NO_ID, SERVER, "il server è stato aggiornato con successo!");
	}
}



void thread_worker(void *arg){
	server_context_t *ctx = arg;

}





int update_rescuer_digital_twin_position(rescuer_digital_twin_t *t){
	if(t->status == IDLE || t->status == ON_SCENE) // se non deve muoversi non faccio nulla
		return NO; 																	// la posizione non va aggiornata perch§e il rescuer non deve muoversi
	
	int d  = MANHATTAN(t->x, 	t->y, 	t->x_destination, 	t->y_destination);
	int dx = MANHATTAN(t->x, 	0, 	 		t->x_destination, 	0);
	int dy = MANHATTAN(0, 	 	t->y, 	0, 							 		t->y_destination);
	int cells_per_second = t->rescuer->speed;

	if(d != 0) 	t-> is_travelling = YES;
	else 				t-> is_travelling = NO;
	
	// se mi basta meno di un secondo sono già arrivato
	if(d < cells_per_second) {			
		t -> x = t -> x_destination;  
		t -> y = t -> y_destination;
	} else {
		// faccio un passo verso la coordinata più lontana attualmente della destinazione
		if(dx > dy){																																		// trovo la coordinata su cui devo fare più strada
			if(dx > cells_per_second)																											// se sono a sinistra della destinazione faccio un passo a destrs, se sono a destra della destinazione faccio un passo a sinistra
				t->x += (t->x < t->x_destination) ? cells_per_second : -cells_per_second;		
			else t->x = t->x_destination;																									// se sono quasi arrivato mi posiziono sull'arrivo
		} else {
			if(dy > cells_per_second)																											// stessa cosa ma per la y
				t->y += (t->y < t->y_destination) ? cells_per_second : -cells_per_second;			
			else t->y = t->y_destination; 						
		}
	}

	if(t->x == t->x_destination && t->y == t->y_destination) { 	// se sono arrivato a destinazione
		t->is_travelling = NO; 																		// non sto più viaggiando
		if(t->status == EN_ROUTE_TO_SCENE) {											// aggiorno lo stato del twin
			t->status = ON_SCENE;
			log_event(t->id, RESCUER_STATUS, "il rescuer è arrivato alla scena dell'emergenza!");
		}
		if(t->status == RETURNING_TO_BASE){
		 	t->status = IDLE;
			log_event(t->id, RESCUER_STATUS, "il rescuer è arrivato alla base :)");
		}																							// sono arrivato a destinazione
	} else log_event(t->id, RESCUER_TRAVELLING_STATUS, "il rescuer sta viaggiando verso la sua destinazione, posizione aggiornata");

	return YES; // ho aggiornato la posizione del gemello digitale
}

void change_rescuer_digital_twin_destination(rescuer_digital_twin_t *t, int x, int y){
	if(t->x == x && t->y == y) return; // se non è cambiata la destinazione non faccio nulla
	t->x_destination = x;
	t->y_destination = y;
}

void send_rescuer_digital_twin_back_to_base(rescuer_digital_twin_t *t){
	if(t->status == IDLE) return; 				// se è già alla base non faccio nulla
	t->status = RETURNING_TO_BASE; 				// cambio lo stato del twin
	log_event(t->id, RESCUER_STATUS, "il rescuer sta tornando alla base");
	change_rescuer_digital_twin_destination(t, t->rescuer->x, t->rescuer->y);
}

void send_rescuer_digital_twin_to_scene(rescuer_digital_twin_t *t, int x, int y){
	switch (t->status) {
		case IDLE: 
			t->status = EN_ROUTE_TO_SCENE;
			log_event(t->id, RESCUER_STATUS, "il rescuer parte dalla base verso la scena dell'emergenza");
			break;
		case EN_ROUTE_TO_SCENE:
			log_event(t->id, RESCUER_STATUS, "il rescuer cambia destinazione verso la scena di un'altra emergenza");
			break; 
		case ON_SCENE:
			log_event(t->id, RESCUER_STATUS, "il rescuer va via dall'emergenza attuale per gestirne un'altra");
			break; 
		case RETURNING_TO_BASE:
			log_event(t->id, RESCUER_STATUS, "il rescuer sta tornando alla base ma cambia destinazione per andare alla scena di un'emergenza");
			break;
		default:
			log_event(t->id, FATAL_ERROR, "stato del rescuer non riconosciuto, impossibile mandarlo alla scena di un'emergenza");
			exit(EXIT_FAILURE);
	}
	change_rescuer_digital_twin_destination(t, x, y);
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
	time_t current_time = time(NULL); 																							
	int height = 0, width = 0, rescuer_count = 0, emergency_types_count = 0;
	rescuer_type_t** rescuer_types = NULL;
	emergency_type_t** emergency_types = NULL;

	// Parsing dei file di configurazione
	log_event(NO_ID, PARSING_STARTED, "Inizio parsing dei file di configurazione");
	parse_env(ctx);
	parse_rescuers(ctx);
	parse_emergencies(ctx);
	log_event(NO_ID, PARSING_ENDED, "Il parsing è terminato con successo!");

	// popolo ctx
	ctx -> current_time = current_time; 
	ctx -> emergency_requests_count = 0; 	// all'inizio non ci sono state ancora richieste
	ctx -> tick = NO;											
	ctx -> tick_count_since_start = 0; 		// il server non ha ancora fatto nessun tick
	ctx -> queue = mallocate_emergency_queue();
	struct mq_attr attr = {
		.mq_flags = 0,
		.mq_maxmsg = MAX_LOG_QUEUE_MESSAGES,
		.mq_msgsize = MAX_LOG_EVENT_LENGTH,
		.mq_curmsgs = 0
	};
	check_error_mq_open(ctx->mq = mq_open(EMERGENCY_QUEUE_NAME, O_CREAT | O_RDONLY, 0644, &attr));
	check_error_mtx_init(mtx_init(&(ctx->clock_mutex), mtx_plain));
	check_error_mtx_init(mtx_init(&(ctx->rescuers_mutex), mtx_plain));
	check_error_cnd_init(cnd_init(&(ctx->clock_updated)));
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
	LOCK(q->mutex);
}

void unlock_queue(emergency_queue_t *q){
	UNLOCK(q->mutex);
}

void lock_emergency_list(emergency_list_t *l){
	LOCK(l->mutex);
}

void unlock_emergency_list(emergency_list_t *l){
	UNLOCK(l->mutex);
}

void lock_rescuer_types(server_context_t *ctx){
	LOCK(ctx->rescuers_mutex);
}

void unlock_rescuer_types(server_context_t *ctx){
	UNLOCK(ctx->rescuers_mutex);
}

void lock_server_clock(server_context_t *ctx){
	LOCK(ctx->clock_mutex);
}

void unlock_server_clock(server_context_t *ctx){
	UNLOCK(ctx->clock_mutex);
}

void tick(server_context_t *ctx){
	ctx->tick = YES; 																				
}

void increment_ticks(server_context_t *ctx){
	ctx->tick_count_since_start++;
}

void untick(server_context_t *ctx){
	ctx->tick = NO; 																				
}

void wait_for_a_tick(server_context_t *ctx){
	cnd_wait(&ctx->clock_updated, &ctx->clock_mutex); 	// attendo che il thread clock mi segnali che il tempo è stato aggiornato
}

int get_server_rescuer_types_count(server_context_t *ctx){
	return ctx -> rescuer_types_count;
}

rescuer_type_t *get_rescuer_type_by_index(server_context_t *ctx, int i){
	return ctx->rescuer_types[i % get_server_rescuer_types_count(ctx)];
}

rescuer_digital_twin_t *get_rescuer_dt_by_index(server_context_t *ctx, int rescuer_type_index, int rescuer_digital_twin_index){
	rescuer_type_t *r = get_rescuer_type_by_index(ctx, rescuer_type_index);
	return (r == NULL) ? NULL : r->twins[rescuer_digital_twin_index];
}

int get_rescuer_type_amount(rescuer_type_t *r){
	return r->amount;
}

int server_is_ticking(server_context_t *ctx){
	return ctx->tick;
}

time_t get_server_current_time(server_context_t *ctx){
	return ctx -> current_time;
}

void set_server_current_time(server_context_t *ctx, time_t new_time){
	ctx -> current_time = new_time;
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
	mtx_destroy(&(ctx->clock_mutex));
	mtx_destroy(&(ctx->rescuers_mutex));
	cnd_destroy(&(ctx->clock_updated));
	free(ctx);
}