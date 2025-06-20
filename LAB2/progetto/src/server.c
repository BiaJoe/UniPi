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
		
		emergency_queue_t *q = get_waiting_emergency_queue_from_context(ctx);
		emergency_t *e = mallocate_emergency(ctx, name, x, y, time);
		emergency_node_t *n = mallocate_emergency_node(e); 	

		lock_queue(q);
		enqueue_emergency_node(q, n);
		cnd_signal(&q->not_empty); 												// segnalo che la coda non è più vuota, un worker thread può processare l'emergenza
		unlock_queue(q);

		lock_list(ctx->emergencies); 												
		append_emergency_node(ctx->emergencies, n); 			// aggiungo l'emergenza alla lista di tutte le emergenze emergenze
		unlock_list(ctx->emergencies); 											

		log_event(NO_ID, EMERGENCY_REQUEST_RECEIVED, "emergenza ricevuta e messa in attesa di essere processata!");
	}
}

// thread funztion per il clock

void thread_clock(void *arg){
	server_context_t *ctx = arg;
	while(1){
		thrd_sleep(SERVER_TICK_TIME, NULL); 									// attendo un tick di tempo del server
		lock_server_clock(ctx);																// blocco il mutex per il tempo del server
		tick(ctx); 																		 				// il sterver ha tickato
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

void promote_waiting_emergencies_if_needed_blocking(server_context_t *ctx){
	emergency_queue_t *q = get_waiting_emergency_queue_from_context(ctx);
	lock_queue(q);
		emergency_node_t *n = q->lists[MIN_EMERGENCY_PRIORITY]->head;
		emergency_node_t *m = n->next;

		// promuovo le emergenze da min priority a medium priority se sono in coda da troppo tempo
		while(m != NULL){																									
			if(promote_to_medium_priority_if_needed(q, n)) {				
				log_event(NO_ID, EMERGENCY_STATUS, "emergenza promossa da priorità minima alla priorità superiore perchè in attesa da troppo tempo");
				n = m;
				if(m) m = m->next; 	// se non siamo all'ultimo nodo (se ci siamo alla prossima guardia del while si esce)
				continue; 
			}		
			n = n->next;
			m = n->next;
		}
	unlock_queue(q);
}

void timeout_waitintg_emergencies_if_needed_blocking(server_context_t *ctx){
	emergency_queue_t *q = get_waiting_emergency_queue_from_context(ctx);
	lock_queue(q);
		emergency_node_t *n = q->lists[MIN_EMERGENCY_PRIORITY]->head;
		emergency_node_t *m = n->next;

		// metto in timeout le emergenze che hanno aspettato troppo
		for(int i = MEDIUM_EMERGENCY_PRIORITY; i <= MAX_EMERGENCY_PRIORITY; i++){
			emergency_node_t *n = q->lists[i]->head;
			while(n != NULL){
				if(timeout_waiting_emergency_if_needed(q, n)) 
					log_event(NO_ID, EMERGENCY_STATUS, "emergenza messa in timeout perchè ha aspettato troppo tempo");
				n = n->next;
			}
		}
		
	unlock_queue(q);
}

void timeout_assigned_emergencies_if_needed_blocking(server_context_t *ctx){
	emergency_queue_t *w = get_working_emergency_queue_from_context(ctx);
	lock_queue(w);
	// metto in timeout le emergenze che non possono ricevere rescuers in tempo
		for(int i = MEDIUM_EMERGENCY_PRIORITY; i <= MAX_EMERGENCY_PRIORITY; i++){
			emergency_node_t *n = w->lists[i]->head;
			while(n != NULL){
				lock_node(n);
				if(timeout_working_emergency_if_needed(w, n)) 
					log_event(NO_ID, EMERGENCY_STATUS, "emergenza messa in timeout perchè non ci sono abbasrtanza rescuers disponibili per arrivare in tempo");
				unlock_node(n);
					n = n->next;
			}
		}
	unlock_queue(w);
}


void reallocate_rescuers_to_assigned_emergencies_if_needed_blocking(server_context_t *ctx){
	// itera le emergenze ad alta priorità
	// cerca quelle che non hanno trovato rescuers
	// cerca i loro rescuers tra quelli liberi e quelli di emergenze minori
	// se li trova tutti mette in pausa quelle emergenze
	// altrimenti le manda in timeout

	emergency_queue_t *waiting_queue = get_working_emergency_queue_from_context(ctx);
	emergency_queue_t *working_queue = get_waiting_emergency_queue_from_context(ctx);

	lock_queue(working_queue);
	for(int i = MAX_EMERGENCY_PRIORITY; i > MIN_EMERGENCY_PRIORITY; i--){
		emergency_node_t *n = working_queue->lists[i]->head;
		while(n != NULL){
			lock_node(n);
			if(n->asks_for_rescuers_from_lower_priorities){

			}
			unlock_node(n);
			n = n->next;
		}
	}

	unlock_queue(working_queue);
}

void thread_updater(void *arg){
	server_context_t *ctx = arg;
	while(1){
		lock_server_clock(ctx); 																					
		while(!server_is_ticking(ctx)) wait_for_a_tick(ctx); 										// attendo che il server ticki				
		untick(ctx); 																														// il server ha tickato, lo sblocco		
		unlock_server_clock(ctx); 																				
		log_event(NO_ID, SERVER, "inizio aggiornamento del server...");
		
		update_rescuers_positions_on_the_map_blocking(ctx); 										// aggiorno le posizioni dei rescuers sulla mappa
		promote_waiting_emergencies_if_needed_blocking(ctx); 					
		timeout_waitintg_emergencies_if_needed_blocking(ctx);
		// timeout_assigned_emergencies_if_needed_blocking(ctx);								// ci pensano gli workers
		reallocate_rescuers_to_assigned_emergencies_if_needed_blocking(ctx); 	// metto in pausa emergenze a bassa priorità per fare spazio per quelle più importanti
		
		log_event(NO_ID, SERVER, "lo stato del server è stato aggiornato con successo!");
	}
}

emergency_node_t *assign_hottest_node(emergency_queue_t *waitq, emergency_queue_t *workq){
	emergency_node_t *n = dequeue_emergency_node(waitq); 											// estraggo il nodo più caldo dalla coda delle emergenze in attesa
	enqueue_emergency_node(workq, n); 																				// lo metto nella coda delle emergenze in lavorazione
	n->emergency->status = ASSIGNED; 																					// cambio lo stato dell'emergenza a ASSIGNED
	return n; 																									
}

is_rescuer_digital_twin_available(rescuer_digital_twin_t *dt){													
	if(dt->status == EN_ROUTE_TO_SCENE || dt->status == ON_SCENE) return NO; 								
	return YES; 																																						
}

rescuer_digital_twin_t *find_nearest_available_rescuer_digital_twin(rescuer_type_t *r, emergency_node_t *n){
	int min_distance = MAX(MAX_X_COORDINATE_ABSOLUTE_VALUE, MAX_Y_COORDINATE_ABSOLUTE_VALUE); 	// inizializzo la distanza minima a un valore molto grande, nessun valore sarà mai così grande
	int d = min_distance; 																																			
	rescuer_digital_twin_t *nearest_dt = NULL; 																									// inizializzo il gemello digitale più vicino a NULL
	for(int i = 0; i < r->amount; i++){ 																												// scorro tutti i gemelli digitali del rescuer type
		rescuer_digital_twin_t *dt = get_rescuer_digital_twin_by_index(r, i); 										
		if(!is_rescuer_digital_twin_available(dt)) continue; 																													
		d = MANHATTAN(dt->x, dt->y, n->emergency->x, n->emergency->y);
		if(d < min_distance){ 																																		
			min_distance = d; 																		
			nearest_dt = dt; 																																				
		}
	}
	if(nearest_dt != NULL) 			// se l'ho trovato abbiamo finito... altrimenti dovremo rubarlo a qualche emrgenza meno importante
		return nearest_dt;

	for
																							
}

int find_and_send_nearest_available_rescuers(emergency_node_t *n){	
	rescuer_digital_twin_t *rescuers[n->emergency->rescuer_count]; 																				// creo un array temporaneo di rescuers della dimensione del numero di rescuers richiesti dall'emergenza
	int rescuer_index = 0; 																																	
	int rescuer_types_amount = n->emergency->type->rescuers_req_number;
	time_t max_time_to_arrive = 0;
	time_t time_to_arrive = ARRIVING_TIME_IF_RESCUERS_WONT_ARRIVE; 																				// inizializzo il tempo massimo di arrivo a un valore invalido
	for(int i = 0; i < rescuer_types_amount; i++){																												// scorro i rescuer types richiesti
		int rescuer_dt_amount = n->emergency->type->rescuers[i]->required_count; 	
		rescuer_type_t *rt = n->emergency->type->rescuers[i]; 																							// prendo il rescuer type i-esimo
		for(int j = 0; j < rescuer_dt_amount; j++){																													// per ognuno, devo trovare i gemelli richiesti
			rescuer_digital_twin_t *dt = find_nearest_available_rescuer_digital_twin(rt, n);
			if(dt == NULL) {
				n->rescuers_are_arriving = NO;
				return NO;
			}																																																	// se non lo trovo è inutile andare avanti
			rescuers[rescuer_index++] = dt; 																																	// lo aggiungo all'array temporaneo di rescuers
		}
	}
	for(int i = 0; i < rescuer_index; i++){ 	
		int s = MANHATTAN(rescuers[i]->x, rescuers[i]->y, n->emergency->x, n->emergency->y);			
		int v = rescuers[i]->rescuer->speed; 					
		if(time_to_arrive = (time_t) ABS(s / v) > max_time_to_arrive) max_time_to_arrive = time_to_arrive; 	// trovo il tempo che ci mette ad arrivare il rescuer più lontano/lento
		n->emergency->rescuer_twins[i] = rescuers[i]; 																											// aggiungo il gemello digitale all'emergency perchè ora sono sicuro che è valido
		send_rescuer_digital_twin_to_scene(rescuers[i], n->emergency); 																			// mando i gemelli sulla scena, ritorno il tempo stimato perchè tutti arrivino
	}
	n->time_estimated_for_rescuers_to_arrive = max_time_to_arrive; 																				// aggiorno il tempo stimato per l'arrivo dei rescuers
	n->rescuers_are_arriving = YES; 																																			// segno che i rescuers stanno arrivando
	return YES; 																																													// ritorno il tempo stimato per l'arrivo dell'ultimo rescuer. Serve per capire se devo o non devo mettere l'emergenza in timeout
}

int find_and_send_nearest_available_and_stealable_rescuers(emergency_node_t *n){
	
}

time_t get_time_before_timeout(emergency_node_t *n){
	switch (n->priority) {
		case MEDIUM_EMERGENCY_PRIORITY: return MAX_TIME_IN_MEDIUM_PRIORITY_BEFORE_TIMEOUT;
		case MAX_EMERGENCY_PRIORITY: 		return MAX_TIME_IN_MAX_PRIORITY_BEFORE_TIMEOUT;
		default: 												return UNDEFINED_TIME_FOR_RESCUERS_TO_ARRIVE;  
	}
}

void timeout_emergency_logging(emergency_t *e){
	if(e->status == TIMEOUT) return; 								// se l'emergenza è già in timeout non faccio nulla
	log_event(NO_ID, EMERGENCY_STATUS, "emergenza messa in timeout perchè i rescuers non hanno tempo di arrivare");
	e->status = TIMEOUT; 														// metto l'emergenza in timeout
}

void wait_until_an_emergency_occurs(emergency_queue_t *waitq){
	cnd_wait(!waitq->is_empty, &waitq->mutex);
}

void thread_worker(void *arg){
	server_context_t *ctx = arg;
	emergency_node_t *n = NULL;
	emergency_queue_t *waiting_queue = get_waiting_emergency_queue_from_context(ctx); 	// estraggo la coda delle emergenze in attesa
	emergency_queue_t *working_queue = get_working_emergency_queue_from_context(ctx); 	// estraggo la coda delle emergenze in che sto processando

	while(1){		
		lock_queue(waiting_queue); 			
		while(is_queue_empty(waiting_queue)) 																							// attendo che ci sia qualcosa da processare
			wait_until_an_emergency_occurs(waiting_queue); 
		lock_queue(working_queue); 									
		n = assign_hottest_node(waiting_queue, working_queue); 														// metto il nodo più caldo nella coda di lavoro	
		log_event(NO_ID, EMERGENCY_REQUEST_PROCESSED, "emergenza assegnata a un thread worker, inizia la ricerca per dei rescuer");
		unlock_queue(working_queue); 	
		unlock_queue(waiting_queue); 	

		// ho il nodo da processare, devo trovare i rescuers
		lock_node(n);	
		lock_rescuer_types(ctx); 																														
		find_and_send_nearest_available_rescuers(n); 						// cerco i rescuers per l'emergenza
		unlock_rescuer_types(ctx); 	

		if(time(NULL) - n->emergency->time + n->time_estimated_for_rescuers_to_arrive > get_time_before_timeout(n)) { 													// se il tempo stimato per l'arrivo dei rescuers è maggiore del tempo massimo prima del timeout
			timeout_emergency_logging(n->emergency);	// metto l'emergenza in timeout
		}
		

		unlock_node(n);
		

	}		
	return;
}






int update_rescuer_digital_twin_position(rescuer_digital_twin_t *t){
	if(t->status == IDLE || t->status == ON_SCENE) // se non deve muoversi non faccio nulla
		return NO; 																	// la posizione non va aggiornata perch§e il rescuer non deve muoversi
	
	int xA, xB, yA, yB, dx, dy, d, cpt, m, vx, vy;

	xA = t->x;													// coordinate attuali
	yA = t->y;
	xB = t->x_destination;							// coordinate obiettivo
	yB = t->y_destination;
	dx = ABS(xA - xB);									// distanza asse X (delta x)
	dy = ABS(yA - yB);									// distanza asse Y (delta y)
	d  = dx + dy;												// uso la formula di Manhattan in due parti perchè mi servono anche i valori intermedi
	
	/*
	come trovare x e y in funzione di c e m, cioè il numero di celle che voglio percorrere e il coefficiente angolare tra i due punti 
	y = mx
	x + y = c
	=> x * (1 + m) = c  =>  x = c / (1 + m)	 =>  y = c - x					
	*/
	
	cpt = t->rescuer->speed; 							// cells per tick
	m   = (dx != 0) ? dy / dx : 0;				// coefficiente angolare della retta da percorrere, se la retta è verticale si applica l'eccezione
	vx  = (dx != 0) ? cpt / (m + 1) : 0;	// celle da percorrere sulla x
	vy  = cpt - vx;												// celle da percorrere sulla y

	if(d < cpt) {													// se mi basta meno di un tick sono già arrivato, aggiorno le coordinate
		t->x = xB;  
		t->y = yB;
		t->is_travelling = NO; 							// non sto più viaggiando
		if(t->status == EN_ROUTE_TO_SCENE) {										
			t->status = ON_SCENE;
			log_event(t->id, RESCUER_STATUS, "il rescuer %s [%d] è arrivato alla scena dell'emergenza (%d, %d) !", t->rescuer->rescuer_type_name, t->id, xB, yB);
		}
		if(t->status == RETURNING_TO_BASE){
		 	t->status = IDLE;
			log_event(t->id, RESCUER_STATUS, "il rescuer %s [%d] è tornato sano e salvo alla base (%d, %d) :)", t->rescuer->rescuer_type_name, t->id, xB, yB);
		}		
	} else {															// non sono ancora arrivato, faccio un passo nella direzione calcolata
		t->x += vx;
		t->y += vy; 
		t-> is_travelling = YES;						// sta ancora viaggiando
		log_event(t->id, RESCUER_TRAVELLING_STATUS, "il rescuer %s [%d] si è spostato da (%d, %d) a (%d, %d)", t->rescuer->rescuer_type_name, t->id, xA, yA, xB, yB);
	}

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
	log_event(t->id, RESCUER_STATUS, "il rescuer  %s [%d] sta tornando alla base", t->rescuer->rescuer_type_name, t->id);
	change_rescuer_digital_twin_destination(t, t->rescuer->x, t->rescuer->y);
}

void send_rescuer_digital_twin_to_scene(rescuer_digital_twin_t *t, emergency_t *e){
	int x = e->x;
	int y = e->y;
	switch (t->status) {
		case IDLE: 
			t->status = EN_ROUTE_TO_SCENE;
			log_event(t->id, RESCUER_STATUS, "il rescuer %s [%d] parte dalla base verso le [%d,%d] %s la scena dell'emergenza", t->rescuer->rescuer_type_name, t->id, x, y, e->type->emergency_desc);
			break;
		case EN_ROUTE_TO_SCENE:
			log_event(t->id, RESCUER_STATUS, "il rescuer %s [%d] cambia destinazione verso le [%d,%d] %s la scena di un'altra emergenza", t->rescuer->rescuer_type_name, t->id, x, y, e->type->emergency_desc);
			break; 
		case ON_SCENE:
			log_event(t->id, RESCUER_STATUS, "il rescuer %s [%d] va via dall'emergenza attuale per gestire [%d,%d] %s", t->rescuer->rescuer_type_name, t->id, x, y, e->type->emergency_desc);
			break; 
		case RETURNING_TO_BASE:
			log_event(t->id, RESCUER_STATUS, "il rescuer %s [%d] sta tornando alla base ma cambia destinazione per andare a gestire [%d,%d] %s", t->rescuer->rescuer_type_name, t->id, x, y, e->type->emergency_desc);
			break;
		default:
			log_event(t->id, FATAL_ERROR, "stato del rescuer %s [%d] non riconosciuto, impossibile mandarlo alla scena di un'emergenza", t->rescuer->rescuer_type_name, t->id);
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
	ctx -> waiting_queue = mallocate_emergency_queue();
	ctx -> working_queue = mallocate_emergency_queue();
	ctx -> emergencies   = mallocate_emergency_list(); 
	
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

	return ctx; 																									// ritorno il contesto del server
}

emergency_type_t **get_emergency_types_from_context(server_context_t *ctx){
	return ctx -> emergency_types;
}

rescuer_type_t **get_rescuer_types_from_context(server_context_t *ctx){
	return ctx -> rescuer_types;
}

emergency_queue_t *get_waiting_emergency_queue_from_context(server_context_t *ctx){
	return ctx -> waiting_queue;
}

emergency_queue_t *get_working_emergency_queue_from_context(server_context_t *ctx){
	return ctx -> working_queue;
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
	ctx->tick_count_since_start++; 	// incremento il contatore dei tick del server perchè è appena avvenuto un tick														
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

int get_server_height(server_context_t *ctx){
	return ctx -> height;
}

int get_server_width(server_context_t *ctx){
	return ctx -> width;
}

void cleanup_server_context(server_context_t *ctx){
	free_rescuer_types(get_rescuer_types_from_context(ctx));
	free_emergency_types(get_emergency_types_from_context(ctx));
	free_emergency_queue(get_waiting_emergency_queue_from_context(ctx));
	mq_close(ctx->mq);
	mq_unlink(EMERGENCY_QUEUE_NAME);
	mtx_destroy(&(ctx->clock_mutex));
	mtx_destroy(&(ctx->rescuers_mutex));
	cnd_destroy(&(ctx->clock_updated));
	free(ctx);
}