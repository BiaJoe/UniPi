#ifndef UTILS_H
#define UTILS_H

#include <string.h>

#include "costants.h"
#include "structs.h"

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)


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

// FUNZIONI UTILI PER MANIPOLARE LE STRUTTURE

rescuer_type_t * get_rescuer_type_by_name(char *name, rescuer_type_t **rescuer_types);
emergency_type_t * get_emergency_type_by_name(char *name, emergency_type_t **emergency_types);
rescuer_request_t * get_rescuer_request_by_name(char *name, rescuer_request_t **rescuers);
char* get_name_of_rescuer_requested(rescuer_request_t *rescuer_request);

#endif