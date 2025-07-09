#include "server.h"




// ----------- funzioni per il thread worker -----------


// aspetta che ci sia almeno un nodo da processare
// prende il nodo a priorità più alta più vecchio (hottest)
// esegue i processi necessari per la sua elaborazione
// ad ogni "step" controlla che l'emergenza non sia stata cancellata o messa in pausa
// se lo è stata agisce di conseguenza e passa alla prossima, altrimenti va avanti su quella
void thread_worker_function(void *arg){
	server_context_t *ctx = arg;
	emergency_node_t *n = NULL;
	emergency_queue_t *waiting_queue = get_waiting_emergency_queue_from_context(ctx); 	// estraggo la coda delle emergenze in attesa
	emergency_queue_t *working_queue = get_working_emergency_queue_from_context(ctx); 	// estraggo la coda delle emergenze in che sto processando

	while(1){		
		// attendo che ci sia qualcosa da processare	
		lock_queue(waiting_queue); 			
		while(waiting_queue->is_empty) 	
			cnd_wait(&waiting_queue->not_empty, &waiting_queue->mutex);

		// c'è almeno un nodo da processare: metto il nodo più caldo nella coda di lavoro	
		lock_queue(working_queue); 									
		n = assign_hottest_node(waiting_queue, working_queue); 	
		unlock_queue(working_queue); 	
		unlock_queue(waiting_queue); 	
		log_event(n->emergency->id, EMERGENCY_STATUS, "inizio a lavorare sull'emergenza %d: %s (%d) [%d, %d] ", n->emergency->id, n->emergency->type->emergency_desc, n->emergency->priority, n->emergency->x, n->emergency->y);

		lock_node(n);

		if (!handle_search_for_rescuers(ctx, n)) 						continue; 
		if (!handle_waiting_for_rescuers_to_arrive(ctx, n)) continue;
		if (!handle_emergency_processing(ctx, n)) 					continue;

		// tutti gli altri casi sono stati gestiti: l'emergenza è completata!

		n->emergency->status = COMPLETED;	
		log_event(n->emergency->id, EMERGENCY_STATUS, "L'emergenza %d: %s (%d) [%d, %d] è stata completata!!! che bello :D. tutti e %d i suoi rescuers sono stati rimandati alla base", n->emergency->id, n->emergency->type->emergency_desc, n->emergency->priority, n->emergency->x, n->emergency->y, n->emergency->rescuer_count);
		move_working_node_to_completed(ctx, n);						

		unlock_node(n);
	}		
	return;
}

int handle_search_for_rescuers(server_context_t *ctx, emergency_node_t *n){
	n->emergency->status = ASKING_FOR_RESCUERS;
	log_event(n->emergency->id, EMERGENCY_STATUS, "Inizia la ricerca dei %d rescuers per l'emergenza %d: %s (%d) [%d, %d]",n->emergency->rescuer_count, n->emergency->id, n->emergency->type->emergency_desc, n->emergency->priority, n->emergency->x, n->emergency->y);
	while (!n->rescuers_found && n->emergency->status != CANCELED) {		// cerco i rescuers ogni tot secondi finchè o li ho trovati o l'emergenza è stata cancellata
		lock_rescuer_types(ctx); 																						
		find_and_send_nearest_rescuers(n, RESCUER_SERARCHING_STEAL_MODE); // cerco rescuer, posso anche rubarli da emergenze meno importanti se ce ne sono
		unlock_rescuer_types(ctx); 		
		timeout_working_emergency_if_needed_logging(n);
		if (n->rescuers_found || n->emergency->status == CANCELED) 
			break; 									
		log_event(n->emergency->id, EMERGENCY_STATUS, "L'emergenza %d: %s (%d) [%d, %d] non puó essere gestita subito, attende di trovare altri rescuers", n->emergency->id, n->emergency->type->emergency_desc, n->emergency->priority, n->emergency->x, n->emergency->y);
		struct timespec ts;
		timespec_get(&ts, TIME_UTC);
		ts.tv_sec += TIME_INTERVAL_BETWEEN_RESCUERS_SEARCH_ATTEMPTS_SECONDS;
		cnd_timedwait(&n->waiting, &n->mutex, &ts);			// aspetto qualche secondo e cerco di nuovo i rescuers
	}
	if (n->rescuers_found)
		return YES;
	if (n->emergency->status == CANCELED)
		cancel_and_unlock_working_node_blocking(ctx, n);
	return NO;
}

int handle_waiting_for_rescuers_to_arrive(server_context_t *ctx, emergency_node_t *n){
	n->emergency->status = WAITING_FOR_RESCUERS;			// tutto ok: i rescuers stanno arrivando
	log_event(n->emergency->id, EMERGENCY_STATUS, "L'emergenza %d: %s (%d) [%d, %d] adesso ha i rescuer necessari in arrivo, si aspetta che arrivino", n->emergency->id, n->emergency->type->emergency_desc, n->emergency->priority, n->emergency->x, n->emergency->y);
	while (!n->rescuers_have_arrived && n->emergency->status != PAUSED && n->emergency->status != CANCELED)
		cnd_wait(&n->waiting, &n->mutex);								// aspetto finchè o arrivano i rescuers o l'emergenza è messa in pausa o cancellata
	if (n->rescuers_have_arrived)
		return YES;
	if (n->emergency->status == PAUSED)
		pause_and_unlock_working_node_blocking(ctx, n);
	if (n->emergency->status == CANCELED)
		cancel_and_unlock_working_node_blocking(ctx, n);
	return NO;
}

int handle_emergency_processing(server_context_t *ctx, emergency_node_t *n){
	n->emergency->status = IN_PROGRESS;								// i rescuer iniziano a lavorare, il thread_updater controllerà se l'emergenza è finita
	log_event(n->emergency->id, EMERGENCY_STATUS, "L'emergenza %d: %s (%d) [%d, %d] ha tutti i rescuers sul posto! Inizia ad essere processata", n->emergency->id, n->emergency->type->emergency_desc, n->emergency->priority, n->emergency->x, n->emergency->y);
	while (n->emergency->status != COMPLETED && n->emergency->status != PAUSED && n->emergency->status != CANCELED)
		cnd_wait(&n->waiting, &n->mutex);								// aspetto finchè o arrivano i rescuers o l'emergenza è messa in pausa o cancellata
	if(n->emergency->status == COMPLETED)
		return YES;
	if (n->emergency->status == PAUSED)
		pause_and_unlock_working_node_blocking(ctx, n);
	if (n->emergency->status == CANCELED)
		cancel_and_unlock_working_node_blocking(ctx, n);
	return NO;
}

void cancel_and_unlock_working_node_blocking(server_context_t *ctx, emergency_node_t *n){
	lock_queue(ctx->working_queue);													
	lock_list(ctx->canceled_emergencies);
	change_emergency_node_list_append(n, ctx->canceled_emergencies);
	unlock_list(ctx->canceled_emergencies);
	unlock_queue(ctx->working_queue);
	unlock_node(n);
}

int pause_and_unlock_working_node_blocking(server_context_t *ctx, emergency_node_t *n){
	lock_queue(ctx->waiting_queue);
	lock_queue(ctx->working_queue);													
	// lock_list(ctx->working_queue->lists[n->emergency->priority]);
	change_emergency_node_list_push(n, ctx->canceled_emergencies);
	// unlock_list(ctx->working_queue->lists[n->emergency->priority]);
	unlock_queue(ctx->working_queue);
	unlock_queue(ctx->waiting_queue);
	unlock_node(n);
}

void send_rescuers_back_to_base_logging(emergency_node_t *n){
	for (int i = 0; i < n->emergency->rescuer_count; i++){
		rescuer_digital_twin_t *dt = n->emergency->rescuer_twins[i];
		send_rescuer_digital_twin_back_to_base_logging(dt);
	}
}

void move_working_node_to_completed(server_context_t *ctx, emergency_node_t *n){
	lock_queue(ctx->working_queue);													
	lock_list(ctx->completed_emergencies);
	change_emergency_node_list_append(n, ctx->completed_emergencies);
	unlock_list(ctx->completed_emergencies);
	unlock_queue(ctx->working_queue);
}

int working_emergency_node_should_timeout(emergency_node_t *n){
	if(!n || n->emergency->priority == MIN_EMERGENCY_PRIORITY || n->emergency->status == TIMEOUT) 	
		return NO;
	int time_before_timeout = get_time_before_emergency_timeout_from_poriority(n->emergency->priority);
	int time_passed_since_request = time(NULL) - n->emergency->time;
	if(!n->rescuers_are_arriving)
		return (time_passed_since_request >= time_before_timeout) ? YES : NO;
	int time_to_wait_for_rescuers = n->time_estimated_for_rescuers_to_arrive;
	return (time_passed_since_request + time_to_wait_for_rescuers >= time_before_timeout) ? YES : NO;
}

void timeout_working_emergency_if_needed_logging(emergency_node_t *n){
	if(!working_emergency_node_should_timeout(n)) 
		return;
	n->emergency->status = TIMEOUT;
	log_event(n->emergency->id, EMERGENCY_STATUS, "L'emergenza %s (%d) [%d, %d] è stata messa in timeout perché le risorse dedicate non riusciranno ad arrivare in tempo", n->emergency->type->emergency_desc, n->emergency->priority, n->emergency->x, n->emergency->y);
}

void change_rescuer_digital_twin_destination(rescuer_digital_twin_t *t, int new_x, int new_y){
	if(!t || t->x == new_x && t->y == new_y) return; 		// se non è cambiata la destinazione non faccio nulla
	change_bresenham_trajectory(t->trajectory, t->x, t->y, new_x, new_y);
	t->is_travelling = YES;
	t->has_arrived = NO;
	t->time_of_arrival = INVALID_TIME;
}

emergency_node_t *assign_hottest_node(emergency_queue_t *waitq, emergency_queue_t *workq){
	emergency_node_t *n = dequeue_emergency_node(waitq); 	// estraggo il nodo più caldo dalla coda delle emergenze in attesa
	enqueue_emergency_node(workq, n); 										// lo metto nella coda delle emergenze in lavorazione
	n->emergency->status = ASSIGNED; 											// cambio lo stato dell'emergenza a ASSIGNED
	return n; 																									
}
