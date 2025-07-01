#ifndef PARSERS_H
#define PARSERS_H

#define _GNU_SOURCE // per usare getline

#include <stdlib.h>
#include <threads.h>
#include "log.h"

// funzioni generali
void parse_rescuers(server_context_t *ctx);
void parse_emergencies(server_context_t *ctx);
void parse_env(server_context_t *ctx);
void my_getline(char **line, size_t *len, FILE *stream);

// funzioni di checking errori ed estrazione valori
int rescuer_type_values_are_illegal(server_context_t *ctx, char *name, int amount, int speed, int x, int y);
int rescuer_request_values_are_illegal(char *rr_name, int required_count, int time_to_manage);
int emergency_values_are_illegal(char *emergency_desc, short priority);
int environment_values_are_illegal(char *queue, int height, int width);
int check_and_log_if_line_is_empty(char *line, char *filename);
void log_and_fail_if_file_line_cant_be_processed(char *line, int line_count, int max_lines, int lines_extracted_so_far, int max_extractable_lines, int max_line_length, char *filename);
int check_and_log_if_emergency_type_already_parsed(emergency_type_t**types, char *desc, int line_count);
void check_and_extract_simple_emergency_type_fields_from_line(char* filename, int line_count, char *line, short *priority, char *desc, char *requests);
void check_and_extract_rescuer_request_fields_from_token(char *filename, int line_count, int requests_parsed_so_far, char*token, char *name, int *count, int *time);
int check_and_log_if_rescuer_request_already_parsed(char*filename, int line_count, rescuer_request_t **requests, char* name);
check_if_rescuer_requested_is_available(char *filename, int line_count, rescuer_type_t **types, char *name, int count);
rescuer_request_t **check_and_extract_rescuer_requests_from_string(rescuer_type_t **rescuer_types, char *filename, int line_count, char *string_of_rescuer_requests, int *rescuer_req_number);



// memoria
rescuer_type_t ** callocate_rescuer_types();
void mallocate_and_populate_rescuer_type(char *name, int amount, int speed, int x, int y, rescuer_type_t **rescuer_types, int i);
void free_rescuer_types(rescuer_type_t **rescuer_types);

emergency_type_t ** callocate_emergency_types();
rescuer_request_t ** callocate_resquer_requests();
void mallocate_and_populate_emergency_type(short priority, char *emergency_desc, int rescuer_req_number, rescuer_request_t **rescuers, emergency_type_t **emergency_types, int i);
void mallocate_and_populate_rescuer_request(char *rr_name, int required_count, int time_to_manage, rescuer_request_t **rescuers, rescuer_type_t **rescuer_types, int i);
emergency_request_t* mallocate_and_populate_emergency_request(char* name, int x, int y, time_t d);

void free_rescuer_requests(rescuer_request_t **rescuer_requests);
void free_emergency_types(emergency_type_t **emergency_types);


#endif