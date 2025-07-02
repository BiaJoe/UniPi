#ifndef PARSERS_H
#define PARSERS_H

#define _GNU_SOURCE // per usare getline

#include <stdlib.h>
#include <threads.h>
#include "log.h"

typedef struct {
	FILE *fp;
	char* filename;			
	char* line;						// la linea che stiamo parsando (init a NULL perchè sarà analizzata da getline())
	size_t len;						// per getline()
	int 	line_number;		// il suo numero		
	int 	parsed_so_far;	// quante entità abbiamo raccolto fin ora
}	parsing_state_t;

typedef struct {
	char queue_name[EMERGENCY_QUEUE_NAME_LENGTH + 1];
	int height;
	int width;
} enviroment_fields_t;

// funzioni generali
void parse_rescuers(server_context_t *ctx);
void parse_emergencies(server_context_t *ctx);
void parse_env(server_context_t *ctx);
int go_to_next_line(parsing_state_t *ps);

// funzioni di checking errori ed estrazione valori
int rescuer_type_values_are_illegal(char *name, int amount, int speed, int x, int y, int maxx, int maxy);
int rescuer_request_values_are_illegal(char *rr_name, int required_count, int time_to_manage);
int emergency_values_are_illegal(char *emergency_desc, short priority);
int environment_values_are_illegal(enviroment_fields_t *e);

int check_and_log_if_line_is_empty(parsing_state_t *ps);
void log_and_fail_if_file_line_cant_be_processed(parsing_state_t *ps, int max_lines, int max_parsable_lines, int max_line_length);
int check_and_log_if_emergency_type_already_parsed(parsing_state_t *ps, emergency_type_t**types, char *desc);
int check_and_log_if_rescuer_request_already_parsed(parsing_state_t *ps, rescuer_request_t **requests, char* name);
void check_if_rescuer_requested_is_available(parsing_state_t *ps, rescuer_type_t **types, char *name, int count);

void check_and_extract_rescuer_type_fields_from_line(parsing_state_t *ps, int maxx, int maxy, char *name, int *amount, int *speed, int *x, int *y);
void check_and_extract_simple_emergency_type_fields_from_line(parsing_state_t *ps, short *priority, char *desc, char *requests);
void check_and_extract_rescuer_request_fields_from_token(parsing_state_t *ps, int requests_parsed_so_far, char*token, char *name, int *count, int *time);
rescuer_request_t **check_and_extract_rescuer_requests_from_string(parsing_state_t *ps, rescuer_type_t **rescuer_types, char *string_of_rescuer_requests, int *rescuer_req_number);
void check_and_extract_env_line_field(parsing_state_t *ps, enviroment_fields_t *e);

// memoria
parsing_state_t *mallocate_parsing_state(char *filename);
void free_parsing_state(parsing_state_t *ps);

rescuer_type_t **callocate_rescuer_types();
rescuer_type_t *mallocate_and_populate_rescuer_type(char *name, int amount, int speed, int x, int y);
rescuer_digital_twin_t **callocate_and_populate_rescuer_digital_twins(rescuer_type_t* r);
rescuer_digital_twin_t *mallocate_rescuer_digital_twin(rescuer_type_t* r);
rescuer_digital_twin_t *mallocate_rescuer_digital_twin(rescuer_type_t* r);
void free_rescuer_types(rescuer_type_t **rescuer_types);

emergency_type_t 	**callocate_emergency_types();
rescuer_request_t **callocate_rescuer_requests();
emergency_type_t 	*mallocate_and_populate_emergency_type(short priority, char *desc, int rrn, rescuer_request_t **rescuers);
rescuer_request_t *mallocate_and_populate_rescuer_request(char *name, int required_count, int time_to_manage, rescuer_type_t *type);
void free_emergency_types(emergency_type_t **emergency_types);
void free_rescuer_requests(rescuer_request_t **rescuer_requests);

emergency_request_t* mallocate_and_populate_emergency_request(char* name, int x, int y, time_t d);



#endif