#ifndef PARSERS_H
#define PARSERS_H

#define _GNU_SOURCE // per usare getline

#include "memory_management.h"
#include "errors.h"



rescuer_type_t ** parse_rescuers(int* rescuer_types);

emergency_type_t ** parse_emergencies(int* emergency_count, rescuer_type_t **rescuer_types);

void check_emergency_type_syntax_and_extract_values(
  int line_count, // serve per il logging
	char *line, 
	short *priority, 
	char *emergency_desc, 
	rescuer_request_t **rescuers,
	int *rescuer_req_number,
	rescuer_type_t **rescuer_types
);

// Funzioni per la gestione dell'ambiente
char* parse_env(int *height, int *width);
void my_getline(char **line, size_t *len, FILE *stream);


#endif