#ifndef UTILS_H
#define UTILS_H

#include <string.h>
#include <math.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "structs.h"

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

#define TRUNCATE_STRING_AT_MAX_LENGTH(s,maxlen) \
	do { \
		if(strlen(s) >= (maxlen)) \
			(s)[(maxlen)-1] = '\0'; \
	} while(0)

//error checking macros (le uniche macro del progetto che non sono in all caps)

#define check_error_minus_one(s,m) 	if ((s) == -1) 		{ perror(m); exit(EXIT_FAILURE); }
#define check_error_nonzero(s,m) 		if ((s) != 0)			{ perror(m); exit(EXIT_FAILURE); }
#define check_error_NULL(s,m) 			if ((s) == NULL) 	{ perror(m); exit(EXIT_FAILURE); }
#define check_error_not(s,m) 				if (!(s)) 				{ perror(m); exit(EXIT_FAILURE); }
#define check_error_condition(c, m) if ((c)) 					{ perror(m); exit(EXIT_FAILURE); }

#define check_error_mq_open(mq) 		if ((mq) == -1) 	{ perror("mq_open"); 		exit(EXIT_FAILURE); }
#define check_error_mq_send(mq) 		if ((mq) == -1) 	{ perror("mq_send"); 		exit(EXIT_FAILURE); }
#define check_error_mq_recieve(mq) 	if ((mq) == -1) 	{ perror("mq_receive"); exit(EXIT_FAILURE); }


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

int my_atoi(char a[]);
void write_line(FILE *f, char *s);


#define ABS(x) ((x) < 0 ? -(x) : (x))
#define MANHATTAN(x1,y1,x2,y2) (ABS((x1) - (x2)) + ABS((y1) - (y2)))


#endif