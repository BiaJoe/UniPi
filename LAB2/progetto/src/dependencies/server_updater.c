#include "server_updater.h"

// ----------- funzioni per il thread updater -----------

int server_updater(void *arg){
	server_context_t *ctx = arg;
	while(!ctx->server_must_stop){
		lock_server_clock(ctx); 																					
		while(!server_is_ticking(ctx)) 
			wait_for_a_tick(ctx); 							// attendo che il server ticki				
		untick(ctx); 													// il server ha tickato, lo sblocco		
		unlock_server_clock(ctx); 																				
		
		log_event(AUTOMATIC_LOG_ID, SERVER, "inizio aggiornamento #%d del server...", ctx->tick_count_since_start);
		lock_rescuers(ctx);
		lock_queue(ctx->waiting_queue);
		lock_queue(ctx->working_queue);
		
		update_rescuers_states_and_positions_on_the_map_logging(ctx); 										
		update_working_emergencies_statuses_blocking(ctx);
		update_waiting_emergency_statuses_blocking(ctx);
		
		unlock_queue(ctx->working_queue);
		unlock_queue(ctx->waiting_queue);
		unlock_rescuers(ctx);
		log_event(AUTOMATIC_LOG_ID, SERVER, "aggiornamento #%d del server eseguito con successo", ctx->tick_count_since_start);
	}
	cancel_all_working_emergencies_signaling(ctx);	// il server si è fermato, devo cancellare le emergenze ancora in elaborazione
	return 0;
}

void send_rescuer_digital_twin_back_to_base_logging(rescuer_digital_twin_t *t){			
	switch (t->status) {
		case IDLE: return;									// se è già alla base non faccio nulla
		case RETURNING_TO_BASE: return;
		case ON_SCENE: 
			log_event(t->id, RESCUER_STATUS, "il rescuer  %s [%d] parte dalla scena dell'emergenza per tornare alla base", t->rescuer->rescuer_type_name, t->id);
			break;
		case EN_ROUTE_TO_SCENE:
			log_event(t->id, RESCUER_STATUS, "il rescuer  %s [%d] stava andando su una scena ma ora torna alla base", t->rescuer->rescuer_type_name, t->id);
			break;
		default: 
			log_event(t->id, RESCUER_STATUS, "il rescuer  %s [%d]  torna alla base", t->rescuer->rescuer_type_name, t->id);
	}
	t->status = RETURNING_TO_BASE; 				// cambio lo stato del twin
	t->time_to_manage = INVALID_TIME;
	
	change_rescuer_digital_twin_destination(t, t->rescuer->x, t->rescuer->y);
}


void update_rescuers_states_and_positions_on_the_map_logging(server_context_t *ctx){
	int amount = get_server_rescuer_types_count(ctx);
	for(int i = 0; i < amount; i++){													// aggiorno le posizioni dei gemelli rescuers
		rescuer_type_t *r = get_rescuer_type_by_index(ctx, i);
		if(r == NULL) continue; 																// se il rescuer type è NULL non faccio nulla (precauzione)
		for(int j = 0; j < get_rescuer_type_amount(r); j++){
			rescuer_digital_twin_t *dt = get_rescuer_digital_twin_by_index(r, j);
			update_rescuer_digital_twin_state_and_position_logging(dt);
		}
	}
}

int update_rescuer_digital_twin_state_and_position_logging(rescuer_digital_twin_t *t){
	if (!t || t->status == IDLE) 		// se non deve muoversi non faccio nulla
		return NO; 										// la posizione non va aggiornata
	
	if(t->status == ON_SCENE){			// se sta gestendo un'emergenza
		if(--(t->time_left_before_it_can_leave_the_scene) > 0)
			return NO;									// deve ancora stare sulla scena dell'emergenza
		else													// ha finito: può tornare alla base!
			send_rescuer_digital_twin_back_to_base_logging(t);
	}

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

	switch (t->status) {
		case EN_ROUTE_TO_SCENE:
			t->time_left_before_it_can_leave_the_scene = t->time_to_manage;
			t->status = ON_SCENE;
			log_event(t->id, RESCUER_STATUS, "il rescuer %s [%d] è arrivato alla scena dell'emergenza (%d, %d) !!!!", t->rescuer->rescuer_type_name, t->id, t->x, t->x);
			return YES;		
		case RETURNING_TO_BASE:
			t->status = IDLE;
			log_event(t->id, RESCUER_STATUS, "il rescuer %s [%d] è tornato sano e salvo alla base (%d, %d) :)", t->rescuer->rescuer_type_name, t->id, t->x, t->x);
			return YES;
		default: log_fatal_error("spostamento rescuer dt");
	}	
}

void timeout_emergency_if_needed_logging(emergency_node_t* n){
	if(!n || n->emergency->status == TIMEOUT)
		return;

	int time_limit = get_time_before_emergency_timeout_from_poriority(n->emergency->priority);
	if(n->emergency->time_spent_existing >= time_limit){
		log_event(AUTOMATIC_LOG_ID, EMERGENCY_STATUS, "Emergenza %d: %s (%d) [%d, %d] messa in timeout perchè il tempo limite è stato superato e non si sono ancora liberate risorse per gestirla!", n->emergency->id, n->emergency->type->emergency_desc, n->emergency->priority, n->emergency->x, n->emergency->y);
		n->emergency->status = TIMEOUT;
		return;
	}

	if(
		n->emergency->status == WAITING_FOR_RESCUERS && 
		n->emergency->time_spent_existing + n->time_estimated_for_rescuers_to_arrive > time_limit
	) {
		log_event(AUTOMATIC_LOG_ID, EMERGENCY_STATUS, "Emergenza %d: %s (%d) [%d, %d] messa in timeout perchè i rescuers non faranno in tempo ad arrivarci! Sono troppo lontani.", n->emergency->id, n->emergency->type->emergency_desc, n->emergency->priority, n->emergency->x, n->emergency->y);
		n->emergency->status = TIMEOUT;
	}
	return;
}	

// se l'emergenza sta cercando rescuers da troppo tempo e sta ingolfando il programma la cancello
// se l'emergenza sta aspettando che i rescuers arrivino controllo se siano arrivati
// se l'ermergenza sta aspettando che i rescuers la processino stando sul posto
// - mando a casa i rescuers che hanno fatto
// - se tutti hanno fatto l'emergenza è completata
// - se ne manca ancora qualcuno l'emergenza non è completata
void update_working_emergency_node_status_signaling_logging(emergency_node_t *n){
	n->emergency->time_since_it_was_assigned++;
	n->emergency->time_spent_existing++;

	timeout_emergency_if_needed_logging(n);
	
	if(n->emergency->status == ASKING_FOR_RESCUERS || n->emergency->status == TIMEOUT){
		if (n->emergency->time_since_it_was_assigned > TIME_BEFORE_AN_EMERGENCY_SHOULD_BE_CANCELLED_TICKS){
		n->emergency->status = CANCELED;
		cnd_signal(&n->waiting);
		}
		return;
	}
	
	int all_rescuers_have_arrived = YES;
	if(n->emergency->status == WAITING_FOR_RESCUERS || n->emergency->status == TIMEOUT){
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

	int all_rescuers_finished_their_work = YES;
	if (n->emergency->status == IN_PROGRESS){
		for (int i = 0; i < n->emergency->rescuer_count; i++){
			rescuer_digital_twin_t *dt = n->emergency->rescuer_twins[i];
			if (dt->status == ON_SCENE){
			 	all_rescuers_finished_their_work = NO;
				break;
			}
		}
		if (all_rescuers_finished_their_work){
			n->emergency->status = COMPLETED;
			cnd_signal(&n->waiting);
		}
		return;
	}
}

int promote_to_medium_priority_if_needed_logging(emergency_queue_t* q, emergency_node_t* n){ // solo da 0 a 1 e non diversamente
	if(!n->emergency->priority == MIN_EMERGENCY_PRIORITY)
		return;
	if(n->emergency->time_spent_existing >= MAX_TIME_IN_MIN_PRIORITY_BEFORE_PROMOTION){
		change_node_priority_list(q, n, MEDIUM_EMERGENCY_PRIORITY);
		log_event(AUTOMATIC_LOG_ID, EMERGENCY_STATUS, "L'emergenza %d: %s (%d) [%d, %d] è stata promossa da priorità minima a media", n->emergency->id, n->emergency->type->emergency_desc, n->emergency->priority, n->emergency->x, n->emergency->y);
	}
}

void update_working_emergencies_statuses_blocking(server_context_t *ctx){
	emergency_queue_t *q = ctx->working_queue;
	lock_queue(q);
	for (int i = MIN_EMERGENCY_PRIORITY; i <= MAX_EMERGENCY_PRIORITY; i++){
		emergency_node_t *n = q->lists[i]->head;
		emergency_node_t *m = (n) ? n->next : NULL;
		while (n != NULL){
			lock_node(n);
			m = n->next; // mi salvo il nodo successivo, perchè potrei dover togliere questo dalla lista in caso di promozione
			update_working_emergency_node_status_signaling_logging(n);
			promote_to_medium_priority_if_needed_logging(q, n);
			unlock_node(n);
			n = m;			 // passo al successivo
		}
	}	
	unlock_queue(q);
}

void update_waiting_emergency_node_status_logging(emergency_node_t *n){
	n->emergency->time_since_started_waiting++;
	n->emergency->time_spent_existing++;
	timeout_emergency_if_needed_logging(n);
}

void update_waiting_emergency_statuses_blocking(server_context_t *ctx){
	emergency_queue_t *q = ctx->waiting_queue;
	lock_queue(q);
	for (int i = MIN_EMERGENCY_PRIORITY; i <= MAX_EMERGENCY_PRIORITY; i++){
		emergency_node_t *n = q->lists[i]->head;
		emergency_node_t *m = (n) ? n->next : NULL;
		while (n != NULL){
			lock_node(n);
			m = n->next; // mi salvo il nodo successivo, perchè potrei dover togliere questo dalla lista in caso di promozione
			update_waiting_emergency_node_status_logging(n);
			promote_to_medium_priority_if_needed_logging(q, n);
			unlock_node(n);
			n = m;			// passo al successivo
		}
	}	
	unlock_queue(q);
}

// scorre le emergenze su cui si sta lavorando 
// cambia lo stato a CANCELED se il mutex edl nodo è libero
// il mutex è libero ogni volta che il thread che elabora il nodo sta aspettando qualcosa
// magari sta aspettando che arrivino i rescuers, questo interrompe l'attesa
void cancel_all_working_emergencies_signaling(server_context_t *ctx){
	emergency_queue_t *q = get_working_emergency_queue_from_context(ctx);
	lock_queue(q);
	for (int i = MIN_EMERGENCY_PRIORITY; i <= MAX_EMERGENCY_PRIORITY; i++){
		emergency_node_t *n = q->lists[i]->head;
		while (n != NULL){
			lock_node(n);
			n->emergency->status = CANCELED; // i thread workers pernseranno a cancellarla
			cnd_signal(&n->waiting);
			unlock_node(n);
			n = n->next;
		}
	}
	unlock_queue(q);
}
