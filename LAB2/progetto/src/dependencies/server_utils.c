
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


