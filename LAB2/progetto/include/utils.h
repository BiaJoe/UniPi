#ifndef UTILS_H
#define UTILS_H

#include <string.h>
#include <math.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <threads.h>
#include <ctype.h>

#include "structs.h"

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

#define TRUNCATE_STRING_AT_MAX_LENGTH(s,maxlen) \
	do { \
		if(strlen(s) >= (maxlen)) \
			(s)[(maxlen)-1] = '\0'; \
	} while(0)

//error checking macros (le uniche macro del progetto che non sono in all caps)

#define check_error(c, m) 								if ((c)) 	{ perror(m); exit(EXIT_FAILURE); }
#define check_error_fopen(fp) 						check_error((fp) == NULL, "fopen")
#define check_error_memory_allocation(p) 	check_error(!(p), "memory allocation")

#define check_error_mq_open(mq) 					check_error((mq) == (mqd_t)-1, "mq_open") 
#define check_error_mq_send(b) 						check_error((b) == -1, "mq_send")
#define check_error_mq_recieve(b) 				check_error((b) == -1, "mq_receive")
		
#define check_error_fork(pid) 						check_error((pid) < 0, "fork_failed")
#define check_error_syscall(call, m)			check_error((call) == -1, m)
#define check_error_mtx_init(call)  			check_error((call) != thrd_success, "mutex init")
#define check_error_cnd_init(call)  			check_error((call) != thrd_success, "mutex init")


// #define check_error_not(s,m) 					check_error(!(s), m)
// #define check_error_nonzero(s,m) 			check_error((s) != 0, m)
// #define check_error_minus_one(s,m) 		check_error((s) == -1, m)

// macros per i processi
// metto due __underscore così non rischio di usare le variabili per sbaglio
#define FORK_PROCESS(child_function_name, parent_function_name) \
	do { 																													\
		pid_t __fork_pid = fork(); 																	\
		check_error_fork(__fork_pid); 															\
		if (__fork_pid == 0) 																				\
			child_function_name(); 																		\
		else																												\
			parent_function_name(); 																	\
	} while(0) 

#define LOCK(m) mtx_lock(&(m))
#define UNLOCK(m) mtx_unlock(&(m))

// macro che permette di scrivere velocemente un comando singolo che richiede lock e unlock di un mutex
// utile perchè cxon questa si scrive meno ed è impossibile dimenticarsi di unlockare
// limnito il numero di comandi a uno solo senza un dowhile perchè non voglio rischiare
// comportamenti inaspettati come exit, return, break
#define LOCK_UNLOCK_1(mutex, expr) \
	do{ \
		LOCK(mutex); \
		expr \
		UNLOCK(mutex); \
	} while(0)


int my_atoi(char a[]);
void write_line(FILE *f, char *s);
int is_line_empty(char *line);

// manipolazione strutture
rescuer_type_t * get_rescuer_type_by_name(char *name, rescuer_type_t **rescuer_types);
emergency_type_t * get_emergency_type_by_name(char *name, emergency_type_t **emergency_types);
rescuer_request_t * get_rescuer_request_by_name(char *name, rescuer_request_t **rescuers);
char* get_name_of_rescuer_requested(rescuer_request_t *rescuer_request);



#define ABS(x) ((x) < 0 ? -(x) : (x))
#define MANHATTAN(x1,y1,x2,y2) (ABS((x1) - (x2)) + ABS((y1) - (y2)))
#define MAX(a,b) ((a) > (b) ? (a) : (b))




#endif