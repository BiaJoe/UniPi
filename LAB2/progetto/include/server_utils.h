#ifndef SERVER_UTILS_H
#define SERVER_UTILS_H

#include "emergency_priority_queue.h"
#include "bresenham.h"

void change_rescuer_digital_twin_destination(rescuer_digital_twin_t *t, int new_x, int new_y);

void lock_rescuer_types(server_context_t *ctx);
void unlock_rescuer_types(server_context_t *ctx);

emergency_type_t **get_emergency_types_from_context(server_context_t *ctx);
rescuer_type_t **get_rescuer_types_from_context(server_context_t *ctx);
emergency_queue_t *get_waiting_emergency_queue_from_context(server_context_t *ctx);
emergency_queue_t *get_working_emergency_queue_from_context(server_context_t *ctx);
int get_server_rescuer_types_count(server_context_t *ctx);
rescuer_type_t *get_rescuer_type_by_index(server_context_t *ctx, int i);
rescuer_digital_twin_t *get_rescuer_dt_by_index(server_context_t *ctx, int rescuer_type_index, int rescuer_digital_twin_index);
int get_rescuer_type_amount(rescuer_type_t *r);
int get_server_height(server_context_t *ctx);
int get_server_width(server_context_t *ctx);


#endif  