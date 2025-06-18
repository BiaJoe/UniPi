#ifndef SERVER_H
#define SERVER_H

#include "emergency_priority_queue.h"
#include "parsers.h"
// #include "debug.h"

#define LOG_IGNORE_EMERGENCY_REQUEST(m) log_event(NO_ID, WRONG_EMERGENCY_REQUEST_IGNORED_SERVER, m)
#define LOG_EMERGENCY_REQUEST_RECIVED() log_event(NO_ID, EMERGENCY_REQUEST_RECEIVED, "emergenza ricevuta e messa in attesa di essere processata!")

#define IS_STOP_MESSAGE(m) (strcmp(m, STOP_MESSAGE_FROM_CLIENT) == 0)

// thread functions:
void recieve_emergency_requests(server_context_t *ctx);
void update_server_context(server_context_t *ctx){


int update_rescuer_digital_twin_position(rescuer_digital_twin_t *t);
int parse_emergency_request(char *message, char* name, int *x, int *y, time_t *timestamp);
int emergency_request_values_are_illegal(server_context_t *ctx, char* name, int x, int y, time_t timestamp);

emergency_type_t **get_emergency_types_from_context(server_context_t *ctx);
int get_server_height(server_context_t *ctx);
int get_server_width(server_context_t *ctx);

void cleanup_server_context(server_context_t *ctx);
void server(void);
void close_server(server_context_t *ctx);


#endif
