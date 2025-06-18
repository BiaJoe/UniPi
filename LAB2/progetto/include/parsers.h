#ifndef PARSERS_H
#define PARSERS_H

#define _GNU_SOURCE // per usare getline

#include <stdlib.h>
#include <threads.h>
#include "log.h"

// funzioni generali
void parse_rescuers(server_context_t *ctx);
void parse_emergencies(server_context_t *ctx);
void check_emergency_type_syntax_and_extract_values(int line_count, char *line,  short *priority,  char *emergency_desc,  rescuer_request_t **rescuers, int *rescuer_req_number, rescuer_type_t **rescuer_types );
void parse_env(server_context_t *ctx);
void my_getline(char **line, size_t *len, FILE *stream);

// funzioni di checking errori
int rescuer_type_values_are_illegal(server_context_t *ctx, char *name, int amount, int speed, int x, int y);
int rescuer_request_values_are_illegal(char *rr_name, int required_count, int time_to_manage);
int emergency_values_are_illegal(char *emergency_desc, short priority);
int environment_values_are_illegal(char *queue, int height, int width);

// memoria
rescuer_type_t ** callocate_rescuer_types();
void mallocate_and_populate_rescuer_type(char *name, int amount, int speed, int x, int y, rescuer_type_t **rescuer_types);
void free_rescuer_types(rescuer_type_t **rescuer_types);

emergency_type_t ** callocate_emergency_types();
rescuer_request_t ** callocate_resquer_requests();
void mallocate_and_populate_emergency_type(short priority, char *emergency_desc, int rescuer_req_number, rescuer_request_t **rescuers, emergency_type_t **emergency_types );
void mallocate_and_populate_rescuer_request( char *rr_name, int required_count, int time_to_manage, rescuer_request_t **rescuers, rescuer_type_t **rescuer_types);
emergency_request_t* mallocate_and_populate_emergency_request(char* name, int x, int y, time_t d);

void free_rescuer_requests(rescuer_request_t **rescuer_requests);
void free_emergency_types(emergency_type_t **emergency_types);




#endif