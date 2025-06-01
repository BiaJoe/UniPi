#ifndef DEBUG_H
#define DEBUG_H

#include "parsers.h"

// Macro per i rescuer

#define R_NAME(i) rescuers[i]->rescuer_type_name
#define R_X(i) rescuers[i]->x
#define R_Y(i) rescuers[i]->y
#define R_AMOUNT(i) rescuers[i]->amount
#define R_SPEED(i) rescuers[i]->speed

// Macro per i gemelli digitali dei rescuer

#define TWIN(i,j) rescuers[i]->twins[j]
#define T_ID(i,j) rescuers[i]->twins[j]->id
#define T_X(i,j) rescuers[i]->twins[j]->x
#define T_Y(i,j) rescuers[i]->twins[j]->y
#define T_STATUS(i,j) rescuers[i]->twins[j]->status

// Macro per le emergenze

#define E_NAME(i) emergency_types[i]->emergency_desc
#define E_PRIORITY(i) emergency_types[i]->priority
#define E_RESCUERS(i,j) emergency_types[i]->rescuers[j]


void print_rescuer_types(rescuer_type_t** rescuers, int rescuer_count);
char * get_status_string(rescuer_digital_twin_t *rescuer_digital_twin);
void print_emergency_types(emergency_type_t** emergency_types, int emergency_count);
void print_env(char* queue, int height, int width);

#endif