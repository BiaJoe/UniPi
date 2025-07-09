#include "server.h"



// ----------- funzioni per il thread updater -----------

// ad ogni tick del clock blocca tutto e aggiorna lo stato delle cose
// è una funzione sequenziale
void thread_updater(void *arg){
	server_context_t *ctx = arg;
	while(1){
		lock_server_clock(ctx); 																					
		while(!server_is_ticking(ctx)) 
			wait_for_a_tick(ctx); 																								// attendo che il server ticki				
		untick(ctx); 																														// il server ha tickato, lo sblocco		
		unlock_server_clock(ctx); 																				
		log_event(AUTOMATIC_LOG_ID, SERVER, "inizio aggiornamento del server...");
		
		lock_rescuers(ctx);
		lock_queue(ctx->waiting_queue);
		lock_queue(ctx->working_queue);
		update_rescuers_positions_on_the_map_logging(ctx); 										
		update_working_emergencies_statuses_blocking(ctx);
		promote_waiting_emergencies_if_needed_logging(ctx); 					
		timeout_waitintg_emergencies_if_needed_logging(ctx);
		cancel_hopeless_emergencies_if_needed_blocking(ctx);
		unlock_queue(ctx->working_queue);
		unlock_queue(ctx->waiting_queue);
		unlock_rescuers(ctx);

		log_event(AUTOMATIC_LOG_ID, SERVER, "lo stato del server è stato aggiornato con successo!");
	}
}


void update_rescuers_positions_on_the_map_logging(server_context_t *ctx){
	int amount = get_server_rescuer_types_count(ctx);
	for(int i = 0; i < amount; i++){													// aggiorno le posizioni dei gemelli rescuers
		rescuer_type_t *r = get_rescuer_type_by_index(ctx, i);
		if(r == NULL) continue; 																// se il rescuer type è NULL non faccio nulla (precauzione)
		for(int j = 0; j < get_rescuer_type_amount(r); j++){
			rescuer_digital_twin_t *dt = get_rescuer_digital_twin_by_index(r, j);
			update_rescuer_digital_twin_position_logging(dt);
		}
	}
}

int update_rescuer_digital_twin_position_logging(rescuer_digital_twin_t *t){
	if(!t || t->status == IDLE || t->status == ON_SCENE) // se non deve muoversi non faccio nulla
		return NO; // la posizione non va aggiornata
	
	t->is_travelling = YES; // ci accede il gemello vede che sta viaggiando
	
	int xA = t->x;
	int yA = t->y;
	int cells_to_walk_on_the_X_axis;
	int cells_to_walk_on_the_Y_axis;
	
	int we_have_arrived = compute_bresenham_step(
		t->x,
		t->y,
		t->trajectory,
		t->rescuer->speed,
		&cells_to_walk_on_the_X_axis,
		&cells_to_walk_on_the_Y_axis
	);

	t->x += cells_to_walk_on_the_X_axis;		// faccio il passo
	t->y += cells_to_walk_on_the_Y_axis;

	if (!we_have_arrived){
		log_event(t->id, RESCUER_TRAVELLING_STATUS, "il rescuer %s [%d] si è spostato da (%d, %d) a (%d, %d)", t->rescuer->rescuer_type_name, t->id, xA, yA, t->x, t->y);
		return YES; 													// ho aggiornato la posizione
	}
	t->is_travelling = NO; 									// siamo arrivati! non stiamo più viaggiando	
	t->has_arrived = YES;	
	t->time_of_arrival = time(NULL);
	if(t->status == EN_ROUTE_TO_SCENE) {										
		t->status = ON_SCENE;
		log_event(t->id, RESCUER_STATUS, "il rescuer %s [%d] è arrivato alla scena dell'emergenza (%d, %d) !!!!", t->rescuer->rescuer_type_name, t->id, t->x, t->x);
	}
	if(t->status == RETURNING_TO_BASE){
		t->status = IDLE;
		log_event(t->id, RESCUER_STATUS, "il rescuer %s [%d] è tornato sano e salvo alla base (%d, %d) :)", t->rescuer->rescuer_type_name, t->id, t->x, t->x);
	}		
	return YES; // ho aggiornato la posizione del gemello digitale
}

int promote_to_medium_priority_if_needed(emergency_queue_t* q, emergency_node_t* n){ // solo da 0 a 1 e non diversamente
	if(time(NULL) - get_emergency_time(n->emergency) >= MAX_TIME_IN_MIN_PRIORITY_BEFORE_PROMOTION){
		change_node_priority_list(q, n, MEDIUM_EMERGENCY_PRIORITY);
		return YES;
	}
	return NO;
}


void promote_waiting_emergencies_if_needed_logging(server_context_t *ctx){
	emergency_queue_t *q = get_waiting_emergency_queue_from_context(ctx);
	emergency_node_t *n = q->lists[MIN_EMERGENCY_PRIORITY]->head;
	emergency_node_t *m = (n != NULL) ? n->next : NULL;

	while(n != NULL){																									
		if(promote_to_medium_priority_if_needed(q, n)) {				
			log_event(AUTOMATIC_LOG_ID, EMERGENCY_STATUS, "emergenza promossa da priorità minima alla priorità superiore perchè in attesa da troppo tempo");
			n = m;
			m = (m != NULL) ? m->next : NULL; 	
			continue; 
		}		
		n = n->next;													// n diventa il prossimo nodo
		m = (n != NULL) ? n->next : NULL;			// m diventa quello dopo ancora
	}
}

int waiting_emergency_node_should_timeout(emergency_node_t *n){
	if(!n || n->emergency->priority == MIN_EMERGENCY_PRIORITY || n->emergency->status == TIMEOUT) return NO;
	int time_since_request = time(NULL) - n->emergency->time;
	int time_before_timeout = get_time_before_emergency_timeout_from_poriority(n->emergency->priority);
	return (time_since_request > time_before_timeout) ? YES : NO; 
}

int timeout_waiting_emergency_if_needed_logging(emergency_node_t* n){
	if(!waiting_emergency_node_should_timeout(n))
		return NO;
	n->emergency->status = TIMEOUT;
	log_event(AUTOMATIC_LOG_ID, EMERGENCY_STATUS, "emergenza messa in timeout perchè ha aspettato troppo tempo");
	return YES;
}	

void timeout_waitintg_emergencies_if_needed_logging(server_context_t *ctx){
	emergency_queue_t *q = get_waiting_emergency_queue_from_context(ctx);
	emergency_node_t *n = q->lists[MIN_EMERGENCY_PRIORITY]->head;
	emergency_node_t *m = n->next;
	for(int i = MEDIUM_EMERGENCY_PRIORITY; i <= MAX_EMERGENCY_PRIORITY; i++){
		emergency_node_t *n = q->lists[i]->head;
		while(n != NULL){
			timeout_waiting_emergency_if_needed_logging(n);
			n = n->next;
		} 
	}
}

void update_emergency_node_status(emergency_node_t *n){
	if(n->emergency->status == ASKING_FOR_RESCUERS){
		if (time(NULL) - n->emergency->time > TIME_BEFORE_AN_EMERGENCY_SHOULD_BE_CANCELLED_SECONDS){
		n->emergency->status = CANCELED;
		cnd_signal(&n->waiting);
		}
		return;
	}
	
	// se l'emergenza sta aspettando i rescuers controllo se sono arrivati
	int all_rescuers_have_arrived = YES;
	if(n->emergency->status == WAITING_FOR_RESCUERS){
		for(int i = 0; i < n->emergency->rescuer_count; i++){
			rescuer_digital_twin_t *dt = n->emergency->rescuer_twins[i];
			if(!(dt->has_arrived && dt->x == n->emergency->x && dt->y == n->emergency->y)){
				all_rescuers_have_arrived = NO;
				break;
			}
		}
		if(all_rescuers_have_arrived){
			n->rescuers_are_arriving = NO;
			n->rescuers_have_arrived = YES;
			cnd_signal(&n->waiting);
		}
		return;
	}

	// se l'ermergenza sta aspettando che i rescuers la processino stando sul posto
	// mando a casa i rescuers che hanno fatto
	// se tutti hanno fatto l'emergenza è completata
	// se ne manca ancora qualcuno l'emergenza non è completata
	int all_rescuers_finished_their_work = YES;
	if (n->emergency->status == IN_PROGRESS){
		for (int i = 0; i < n->emergency->rescuer_count; i++){
			rescuer_digital_twin_t *dt = n->emergency->rescuer_twins[i];
			if (dt->status == ON_SCENE && time(NULL) - dt->time_of_arrival < dt->time_to_manage)
				all_rescuers_finished_their_work = NO;
			else if (dt->status == ON_SCENE)
				send_rescuer_digital_twin_back_to_base_logging(dt);
		}
		if (all_rescuers_finished_their_work){
			n->emergency->status = COMPLETED;
			cnd_signal(&n->waiting);
		}
		return;
	}
}

void update_working_emergencies_statuses_blocking(server_context_t *ctx){
	emergency_queue_t *q = get_working_emergency_queue_from_context(ctx);
	lock_queue(q);
	for (int i = MIN_EMERGENCY_PRIORITY; i <= MAX_EMERGENCY_PRIORITY; i++){
		emergency_node_t *n = q->lists[i]->head;
		while (n != NULL){
			lock_node(n);
			update_emergency_node_status(n);
			unlock_node(n);
		}
	}	
	unlock_queue(q);
}

// scorre le emergenze su cui si sta lavorando 
// se un'emergenza di priorità non massima occupa risorse da più di tot secondi si cancella
// questa manovra serve per non rischiare di tenere un'emergenza in attesa in un thread all'infinito
// in circostanze normali con numeri non altissimi di emergenze le cancellazioni non sono necessarie
void cancel_hopeless_working_emergencies_if_needed_blocking_signaling(server_context_t *ctx){
	emergency_queue_t *q = get_working_emergency_queue_from_context(ctx);
	lock_queue(q);
	for (int i = MIN_EMERGENCY_PRIORITY; i < MAX_EMERGENCY_PRIORITY; i++){
		emergency_node_t *n = q->lists[i]->head;
		while (n != NULL){
			lock_node(n);
			
			unlock_node(n);
		}
	}
	unlock_queue(q);
}
