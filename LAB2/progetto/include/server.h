#ifndef SERVER_H
#define SERVER_H

#include "emergency_priority_queue.h"
#include "debug.h"

emergency_type_t **get_emergency_types_from_context(server_context_t *ctx);
int get_server_height(server_context_t *ctx);
int get_server_width(server_context_t *ctx);

void cleanup_server_context(server_context_t *ctx);
void server(void);


#endif
