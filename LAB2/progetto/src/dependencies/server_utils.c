
#include "server_utils.h"

// serve a mandare un rescuer in un posto che vogliamo
// la usano più file, wrappata in altre funzioni per facilitare il logging
void change_rescuer_digital_twin_destination(rescuer_digital_twin_t *t, int new_x, int new_y){
	if(!t || t->x == new_x && t->y == new_y) return; 		// se non è cambiata la destinazione non faccio nulla
	change_bresenham_trajectory(t->trajectory, t->x, t->y, new_x, new_y);
	t->is_travelling = YES;
	t->has_arrived = NO;
	t->time_left_before_it_can_leave_the_scene = INVALID_TIME;
}

// locks e unlocks

void lock_rescuer_types(server_context_t *ctx){
	LOCK(ctx->rescuers_mutex);
}

void unlock_rescuer_types(server_context_t *ctx){
	UNLOCK(ctx->rescuers_mutex);
}

// accesso semplificato a variabili

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




