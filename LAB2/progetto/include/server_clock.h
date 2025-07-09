
#ifndef SERVER_CLOCK_H
#define SERVER_CLOCK_H

#include "utils.h"

// tick del server, 1 secondo, ma può essere cambiato per aumentare/diminuire la velocità del server
#define SERVER_TICK_TIME &(struct timespec){.tv_sec = 1, .tv_nsec = 0} 	

int server_clock(void *arg);

void untick(server_context_t *ctx);
void wait_for_a_tick(server_context_t *ctx);
int server_is_ticking(server_context_t *ctx);

void lock_server_clock(server_context_t *ctx);
void unlock_server_clock(server_context_t *ctx);

#endif