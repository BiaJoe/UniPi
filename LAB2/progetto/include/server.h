#ifndef SERVER_H
#define SERVER_H

#include "parsers.h"
#include "logger.h"
#include "debug.h"

typedef struct {
	// interi per tenere traccia di cosa succede
	int height;
	int width;
	int rescuer_count;
	int emergency_types_count;
	int emergency_requests_count;

	// puntatori alle strutture da manipolare
	rescuer_type_t** rescuer_types;
  emergency_type_t** emergency_types;
	// coda per ricevere le richieste di emergenza
	mqd_t mq;
} server_context_t;

void cleanup_server_context(server_context_t *ctx);
void server(void);


#endif
