#ifndef SERVER_H
#define SERVER_H

#include "utils.h"
#include "emergency_priority_queue.h"
#include "parsers.h"

#define THREAD_POOL_SIZE 10

#define RESCUER_SERARCHING_FAIR_MODE 'f'
#define RESCUER_SERARCHING_STEAL_MODE 's'
#define TIME_INTERVAL_BETWEEN_RESCUERS_SEARCH_ATTEMPTS_SECONDS 3
#define TIME_BEFORE_AN_EMERGENCY_SHOULD_BE_CANCELLED_SECONDS 300

#define LOG_IGNORE_EMERGENCY_REQUEST(m) log_event(NO_ID, WRONG_EMERGENCY_REQUEST_IGNORED_SERVER, m)
#define LOG_EMERGENCY_REQUEST_RECIVED() log_event(NO_ID, EMERGENCY_REQUEST_RECEIVED, "emergenza ricevuta e messa in attesa di essere processata!")
#define IS_STOP_MESSAGE(m) (strcmp(m, STOP_MESSAGE_FROM_CLIENT) == 0)
#define SERVER_TICK_TIME &(struct timespec){.tv_sec = 1, .tv_nsec = 0} 	// tick del server, 1 secondo, ma può essere cambiato per aumentare/diminuire la velocità del server
#define ARRIVING_TIME_IF_RESCUERS_WONT_ARRIVE -1


void server(void);
void close_server(server_context_t *ctx);

void lock_rescuer_types(server_context_t *ctx);
void unlock_rescuer_types(server_context_t *ctx);
void lock_server_clock(server_context_t *ctx);
void unlock_server_clock(server_context_t *ctx);
server_context_t *mallocate_server_context();
void cleanup_server_context(server_context_t *ctx);
int get_time_before_emergency_timeout_from_poriority(int p);
void timeout_emergency_logging(emergency_t *e);
int is_rescuer_digital_twin_available(rescuer_digital_twin_t *dt);
rescuer_digital_twin_t *find_nearest_available_rescuer_digital_twin(rescuer_type_t *r, emergency_node_t *n);
int is_rescuer_digital_twin_stealable(rescuer_digital_twin_t *dt, emergency_type_t *stealer_emergency);
rescuer_digital_twin_t *try_to_find_nearest_rescuer_from_less_important_emergency(rescuer_type_t *r, emergency_node_t *n);
void pause_emergency_blocking_signaling_logging(emergency_node_t *n);
int find_and_send_nearest_rescuers(emergency_node_t *n, char mode);
void send_rescuer_digital_twin_back_to_base_logging(rescuer_digital_twin_t *t);
void send_rescuer_digital_twin_to_scene_logging(rescuer_digital_twin_t *t, emergency_node_t *n);
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

void thread_clock(void *arg);
void tick(server_context_t *ctx);
void untick(server_context_t *ctx);
void wait_for_a_tick(server_context_t *ctx);
int server_is_ticking(server_context_t *ctx);

void thread_receiver(server_context_t *ctx);
int parse_emergency_request(char *message, char* name, int *x, int *y, time_t *timestamp);
int emergency_request_values_are_illegal(server_context_t *ctx, char* name, int x, int y, time_t timestamp);

void thread_updater(void *arg);
void update_rescuers_positions_on_the_map_logging(server_context_t *ctx);
int update_rescuer_digital_twin_position_logging(rescuer_digital_twin_t *t);
int promote_to_medium_priority_if_needed(emergency_queue_t* q, emergency_node_t* n){;
void promote_waiting_emergencies_if_needed_logging(server_context_t *ctx);
int waiting_emergency_node_should_timeout(emergency_node_t *n);
int timeout_waiting_emergency_if_needed_logging(emergency_node_t* n);
void timeout_waitintg_emergencies_if_needed_logging(server_context_t *ctx);
void update_emergency_node_status(emergency_node_t *n);
void update_working_emergencies_statuses_blocking(server_context_t *ctx);
void cancel_hopeless_working_emergencies_if_needed_blocking_signaling(server_context_t *ctx);

void thread_worker_function(void *arg);
int handle_search_for_rescuers(server_context_t *ctx, emergency_node_t *n);
int handle_waiting_for_rescuers_to_arrive(server_context_t *ctx, emergency_node_t *n);
int handle_emergency_processing(server_context_t *ctx, emergency_node_t *n);
void cancel_and_unlock_working_node_blocking(server_context_t *ctx, emergency_node_t *n);
int pause_and_unlock_working_node_blocking(server_context_t *ctx, emergency_node_t *n);
void send_rescuers_back_to_base_logging(emergency_node_t *n);
void move_working_node_to_completed(server_context_t *ctx, emergency_node_t *n);
int working_emergency_node_should_timeout(emergency_node_t *n);
void timeout_working_emergency_if_needed_logging(emergency_node_t *n);
void change_rescuer_digital_twin_destination(rescuer_digital_twin_t *t, int new_x, int new_y);



#endif
