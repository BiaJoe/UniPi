#include "server.h"

// ----------- lock e unlock ----------- 

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



// ----------- gestione del server context: inizializzazione, pulizia e accesso a variabili -----------

// faccio il parsing dei file
// ottengo numero e puntatore ad emergenze e rescuers
// creo la coda di messaggi ricevuti dal client
// inizializzo i mutex
server_context_t *mallocate_server_context(){
	server_context_t *ctx = (server_context_t *)malloc(sizeof(server_context_t));	
	time_t current_time = time(NULL); 																							
	int height = 0, width = 0, rescuer_count = 0, emergency_types_count = 0;
	rescuer_type_t** rescuer_types = NULL;
	emergency_type_t** emergency_types = NULL;

	// Parsing dei file di configurazione
	log_event(AUTOMATIC_LOG_ID, PARSING_STARTED, "Inizio parsing dei file di configurazione");
	parse_env(ctx);
	parse_rescuers(ctx);
	parse_emergencies(ctx);
	log_event(AUTOMATIC_LOG_ID, PARSING_ENDED, "Il parsing è terminato con successo!");

	// popolo ctx
	ctx -> current_time = current_time; 
	ctx -> emergency_requests_count = 0; 	// all'inizio non ci sono state ancora richieste
	ctx -> valid_emergency_request_count = 0;
	ctx -> tick = NO;											
	ctx -> tick_count_since_start = 0; 		// il server non ha ancora fatto nessun tick
	ctx -> waiting_queue = mallocate_emergency_queue();
	ctx -> working_queue = mallocate_emergency_queue();
	ctx -> completed_emergencies = mallocate_emergency_list(); 
	ctx -> canceled_emergencies = mallocate_emergency_list();
	
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

int get_server_height(server_context_t *ctx){
	return ctx -> height;
}

int get_server_width(server_context_t *ctx){
	return ctx -> width;
}


int get_time_before_emergency_timeout_from_poriority(int p){
	switch (p) {
		case MEDIUM_EMERGENCY_PRIORITY: return MAX_TIME_IN_MEDIUM_PRIORITY_BEFORE_TIMEOUT; 
		case MAX_EMERGENCY_PRIORITY: 		return MAX_TIME_IN_MAX_PRIORITY_BEFORE_TIMEOUT; 
		default:												return UNDEFINED_TIME_FOR_RESCUERS_TO_ARRIVE;		
	}
}

void timeout_emergency_logging(emergency_t *e){
	if(e->status == TIMEOUT) return; 								// se l'emergenza è già in timeout non faccio nulla
	log_event(AUTOMATIC_LOG_ID, EMERGENCY_STATUS, "emergenza messa in timeout perchè i rescuers non hanno tempo di arrivare");
	e->status = TIMEOUT; 														// metto l'emergenza in timeout
}




// ----------- funzioni di ricerca rescuers -----------

int is_rescuer_digital_twin_available(rescuer_digital_twin_t *dt){			
	return (dt->status == IDLE || dt->emergency_node->emergency->status == PAUSED) ? YES : NO;	// l'ho incapsulato in una funzione perchè così è più semplice cambiare i requisiti, ad esempio potremmo voler chiamare i rescuer anche quando tornano alla base 																																					
}

rescuer_digital_twin_t *find_nearest_available_rescuer_digital_twin(rescuer_type_t *r, emergency_node_t *n){
	int min_distance = MAX(MAX_X_COORDINATE_ABSOLUTE_VALUE, MAX_Y_COORDINATE_ABSOLUTE_VALUE); 	// inizializzo la distanza minima a un valore molto grande, nessun valore sarà mai così grande
	int d = min_distance; 																																			
	rescuer_digital_twin_t *nearest_dt = NULL; 																									// inizializzo il gemello digitale più vicino a NULL
	for (int i = 0; i < r->amount; i++) { 																											// scorro tutti i gemelli digitali del rescuer type
		rescuer_digital_twin_t *dt = get_rescuer_digital_twin_by_index(r, i); 										
		if (!is_rescuer_digital_twin_available(dt)) 
			continue; 																													
		d = MANHATTAN(dt->x, dt->y, n->emergency->x, n->emergency->y);
		if (d < min_distance) { 																																		
			min_distance = d; 																		
			nearest_dt = dt; 																																				
		}
	}
	return nearest_dt;																				
}

int is_rescuer_digital_twin_stealable(rescuer_digital_twin_t *dt, emergency_type_t *stealer_emergency){
	return (
		(dt->status == EN_ROUTE_TO_SCENE || dt->status == ON_SCENE) &&
		dt->emergency_node->emergency->priority < stealer_emergency->priority
	) ? YES : NO;
}

int rescuer_digital_twin_must_be_stolen(rescuer_digital_twin_t *dt){
	return (
		(dt->status == EN_ROUTE_TO_SCENE || dt->status == ON_SCENE) &&
		(dt->emergency_node->emergency->status == WAITING_FOR_RESCUERS || dt->emergency_node->emergency->status == IN_PROGRESS)
	) ? YES : NO;
}

rescuer_digital_twin_t *try_to_find_nearest_rescuer_from_less_important_emergency(rescuer_type_t *r, emergency_node_t *n){
	int min_distance = MAX(MAX_X_COORDINATE_ABSOLUTE_VALUE, MAX_Y_COORDINATE_ABSOLUTE_VALUE); 	// inizializzo la distanza minima a un valore molto grande, nessun valore sarà mai così grande
	int d = min_distance; 																																			
	rescuer_digital_twin_t *nearest_dt = NULL; 		
	for (int i = 0; i < r->amount; i++) { 																											// scorro tutti i gemelli digitali del rescuer type
		rescuer_digital_twin_t *dt = get_rescuer_digital_twin_by_index(r, i); 										
		if (!is_rescuer_digital_twin_stealable(dt, n->emergency)) 
			continue; 																													
		d = MANHATTAN(dt->x, dt->y, n->emergency->x, n->emergency->y);
		if (d < min_distance) { 																																		
			min_distance = d; 																		
			nearest_dt = dt; 																																				
		}
	}
	return nearest_dt;		
}

// cambia lo stato dell'emergenza a paused
// agisce su un nodo che può essere "derubato"
// il nodo è nel suo thread che sta aspettando che i suoi gemelli arrivino al punto dell'emergenza
// oppure aspetta che abbiano finito di stare lí
void pause_emergency_blocking_signaling_logging(emergency_node_t *n){ 
	lock_node(n);
	emergency_t *e = n->emergency;
	if (e->status == PAUSED){					// se l'emergenza è già in pausa non faccio niente (misura di sicurezza per non mettere in pausa un'emergenza e loggare l'evento più di una volta)
		unlock_node(n);
		return;
	}
	e->status = PAUSED;
	n->rescuers_are_arriving = NO;
	n->rescuers_have_arrived = NO;
	n->rescuers_found = NO;
	n->time_estimated_for_rescuers_to_arrive = INVALID_TIME;
	cnd_signal(&n->waiting);
	unlock_node(n);
	log_event(e->id, EMERGENCY_STATUS, "Emergenza %s (%d) [%d, %d] messa in pausa", e->type->emergency_desc, e->priority, e->x, e->y);
}

// cerca tra i rescuer disponibili e li assegna se li trova tutti
// nella fase di ricerca non apporta nessuna modifica ai rescuer o ai loro nodi
// se ha trovato abbastanza rescuer allora passa alla fase di invio
// se è in modalità STEAL può mettere in pausa emergenze e "rubare" i loro rescuer
int find_and_send_nearest_rescuers(emergency_node_t *n, char mode){	
	int we_can_steal;
	switch (mode) {
		case RESCUER_SERARCHING_FAIR_MODE: 	we_can_steal = NO; 	break;
		case RESCUER_SERARCHING_STEAL_MODE: we_can_steal = YES; break;
		default: we_can_steal = NO;
	}

	rescuer_digital_twin_t *rescuers[n->emergency->rescuer_count]; 												// creo un array temporaneo di rescuers della dimensione del numero di rescuers richiesti dall'emergenza
	int rescuer_index = 0; 																																	
	int rescuer_types_amount = n->emergency->type->rescuers_req_number;
	int max_time_to_arrive = 0;
	int time_to_arrive = INVALID_TIME; 												// inizializzo il tempo massimo di arrivo a un valore invalido
	
	// parte della funzione che cerca i rescuer (e ritorna se non li trova)
	for(int i = 0; i < rescuer_types_amount; i++){																				// scorro i rescuer types richiesti
		int rescuer_dt_amount = n->emergency->type->rescuers[i]->required_count; 	
		rescuer_type_t *rt = n->emergency->type->rescuers[i]->type; 												// prendo il rescuer type i-esimo
		for(int j = 0; j < rescuer_dt_amount; j++){																					// per ognuno, devo trovare i gemelli richiesti
			rescuer_digital_twin_t *dt = find_nearest_available_rescuer_digital_twin(rt, n);
			// un rescuer libero non l'ho trovato, magari posso rubarne uno se sono nella modalitàRESCUER_SERARCHING_STEAL_MODE 
			if(we_can_steal && dt == NULL)
				dt = find_nearest_stealable_rescuer_digital_twin(rt, n);
			if (dt == NULL) {
				n->rescuers_are_arriving = NO;
				n->rescuers_found = NO;
				return NO;													// se non lo trovo è inutile andare avanti
			}																			
			rescuers[rescuer_index++] = dt; 			// lo aggiungo all'array temporaneo di rescuers
		}
	}

	// parte della funzione che assegna e invia i rescuer (se prima li ha trovati)
	for(int i = 0; i < rescuer_index; i++){ 	
		rescuer_digital_twin_t *dt = rescuers[i];
		if (we_can_steal && rescuer_digital_twin_must_be_stolen(dt))
			pause_emergency_blocking_signaling_logging(n);
		int s = MANHATTAN(dt->x, dt->y, n->emergency->x, n->emergency->y);			
		int v = dt->rescuer->speed; 			
		time_to_arrive = ABS(s / v) + 1;
		if (time_to_arrive > max_time_to_arrive) 		// trovo il tempo che ci mette ad arrivare il rescuer più lontano/lento
			max_time_to_arrive = time_to_arrive; 
		n->emergency->rescuer_twins[i] = dt; 															// aggiungo il gemello digitale all'emergency perchè ora sono sicuro che è valido
		send_rescuer_digital_twin_to_scene_logging(dt, n); 								// mando i gemelli sulla scena, ritorno il tempo stimato perchè tutti arrivino
	}
	
	n->time_estimated_for_rescuers_to_arrive = max_time_to_arrive; 			// aggiorno il tempo stimato per l'arrivo dei rescuers
	n->rescuers_found= YES;
	n->rescuers_are_arriving = YES; 																		// segno che i rescuers stanno arrivando
	return YES; 																												// ritorno il tempo stimato per l'arrivo dell'ultimo rescuer. Serve per capire se devo o non devo mettere l'emergenza in timeout
}









// ----------- funzioni di spostamento rescuers -----------


void send_rescuer_digital_twin_back_to_base_logging(rescuer_digital_twin_t *t){
	if(t->status == IDLE) return; 				// se è già alla base non faccio nulla
	t->status = RETURNING_TO_BASE; 				// cambio lo stato del twin
	t->time_to_manage = INVALID_TIME;
	log_event(t->id, RESCUER_STATUS, "il rescuer  %s [%d] sta tornando alla base", t->rescuer->rescuer_type_name, t->id);
	change_rescuer_digital_twin_destination(t, t->rescuer->x, t->rescuer->y);
}

void send_rescuer_digital_twin_to_scene_logging(rescuer_digital_twin_t *t, emergency_node_t *n){
	t->emergency_node = n; // il rescuer prende l'emergenza verso cui deve andare
	emergency_t *e = n->emergency;
	rescuer_request_t *request = get_rescuer_request_by_name(t->rescuer->rescuer_type_name, e->type->rescuers);
	t->time_to_manage = request->time_to_manage;	// ottengo quanto devo aspettare sul posto dell'emergenza verso cui sto andando
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
		case RETURNING_TO_BASE: // caso contemplato in questa funzione ma non verificato perchè le specifiche del progetto non lo contemplano. L'ho messo per completezza e per facilitare un eventuale cambio futuro delle regole
			log_event(t->id, RESCUER_STATUS, "il rescuer %s [%d] sta tornando alla base ma cambia destinazione per andare a gestire [%d,%d] %s", t->rescuer->rescuer_type_name, t->id, x, y, e->type->emergency_desc);
			break;
		default:
			log_event(t->id, FATAL_ERROR, "stato del rescuer %s [%d] non riconosciuto, impossibile mandarlo alla scena di un'emergenza", t->rescuer->rescuer_type_name, t->id);
			exit(EXIT_FAILURE);
	}

	change_rescuer_digital_twin_destination(t, x, y);
}	