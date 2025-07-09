#include "bresenham.h"

bresenham_trajectory_t *mallocate_bresenham_trajectory(){
	bresenham_trajectory_t *b = (bresenham_trajectory_t *)malloc(sizeof(bresenham_trajectory_t));
	check_error_memory_allocation(b);
	return b;
}


// funzione riciclabile per ogni entità che segue la linea di bresenham. 
int compute_bresenham_step(int x, int y, bresenham_trajectory_t *trajectory, int cells_per_step, int *x_step, int *y_step){
	if(!trajectory) return NO;
	if(cells_per_step < 0) return YES;

	int xA = x;
	int yA = y;
	int xB = trajectory->x_target;
	int yB = trajectory->y_target;
	int dx = trajectory->dx;
	int dy = trajectory->dy;
	int sx = trajectory->sx;
	int sy = trajectory->sy;
	
	for (int i = 0; i < cells_per_step; i++){			// faccio un passo alla volta percorrendo la linea di Bresenham 
		if (xA == xB && yA == yB) 									// siamo arrivati
			return YES;				
		int e2 = 2 * trajectory->err;								// l'errore serve a dirci se siamo più lontani sulla x o sulla y 
		if (e2 >= -dy) {														// se siamo più lontani sulla x facciamo un passo sulla x
			trajectory->err -= dy;										// aggiorno l'errore 
			xA += sx;																	// faccio un passo sull'asse x
			(*x_step) += sx;													// aggiorno il numero di passi fatti sull'asse x
		}
		else if (e2 <= dx) {												// se invece siamo più lontani sulla y si fa la stessa cosa ma sulla y
			trajectory->err += dx;
			yA += sy;		
			(*y_step) += sy;					
		}
	}

	return (xA == xB && yA == yB);								// siamo arrivati?
}

void change_bresenham_trajectory(bresenham_trajectory_t *t, int current_x, int current_y, int new_x, int new_y){
	if(!t) return;
	t->x_target = new_x;							// le coordinate da raggiungere cambiano
	t->y_target = new_y;							// con loro si aggiorna il resto della traiettoria
	t->dx = ABS(new_x - current_x);				
	t->dy = ABS(new_y - current_y);
	t->sx = (current_x < new_x) ? 1 : -1;
	t->sy = (current_y < new_y) ? 1 : -1;
	t->err = t->dx - t->dy;
}