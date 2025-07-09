#ifndef SERVER_H
#define SERVER_H

#include "server_updater.h"
#include "emergency_handler.h"
#include "parsers.h"


#define THREAD_POOL_SIZE 10

#define LOG_IGNORE_EMERGENCY_REQUEST(m) log_event(NO_ID, WRONG_EMERGENCY_REQUEST_IGNORED_SERVER, m)
#define LOG_EMERGENCY_REQUEST_RECIVED() log_event(NO_ID, EMERGENCY_REQUEST_RECEIVED, "emergenza ricevuta e messa in attesa di essere processata!")
#define IS_STOP_MESSAGE(m) (strcmp(m, STOP_MESSAGE_FROM_CLIENT) == 0)


// server.c
void server(void);
void close_server(server_context_t *ctx);
server_context_t *mallocate_server_context();
void cleanup_server_context(server_context_t *ctx);


#endif
