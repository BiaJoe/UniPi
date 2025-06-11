#ifndef EMERGENCY_PRIORITY_QUEUE_H
#define EMERGENCY_PRIORITY_QUEUE_H

#include "parsers.h"
#include "logger.h"

#define MAX_TIME_IN_0_PRIORITY_BEFORE_PROMOTION	120
#define MAX_TIME_IN_1_PRIORITY_BEFORE_TIMEOUT		30
#define MAX_TIME_IN_2_PRIORITY_BEFORE_TIMEOUT		10


// valori della richiesta -> puntatore a struttura emergenza
emergency_t *mallocate_emergency(server_context_t *ctx, char* name, int x, int y, time_t timestamp);
void free_emergency(emergency_t* e);

#define INVALID_EMERGENCY_PROPERTY_NUMBER 	-1
#define INVALID_EMERGENCY_PROPERTY_POINTER 	NULL

int get_emergency_priority(emergency_t* e);
int get_emergency_x(emergency_t* e);
int get_emergency_y(emergency_t* e);
int get_emergency_rescuer_req_number(emergency_t* e);
rescuer_request_t **get_emergency_resquer_requests(emergency_t* e);
emergency_status_t get_emergency_status(emergency_t* e);
time_t get_emergency_time(emergency_t* e);

// emergenza -> nodo della lista
emergency_node_t* mallocate_emergency_node(emergency_t *e);
void free_emergency_node(emergency_node_t* n);

emergency_list_t *mallocate_emergency_list();
void free_emergency_list(emergency_list_t *el);

emergency_queue_t *mallocate_emergency_queue();
void free_emergency_queue(emergency_queue_t *q);


// inserisce il nodo nella lista
void append_emergency_node(emergency_list_t* el, emergency_node_t* node);

emergency_node_t* decapitate_emergency_list(emergency_list_t* el);

int is_the_first_node_of_the_list(emergency_node_t* node);
int is_the_last_node_of_the_list(emergency_node_t* node);

// estrae il nodo richiesto dalla coda
void remove_emergency_node_from_its_list(emergency_node_t* node);

// inserisce l'emergenza nella queue
void enqueue_emergency(emergency_queue_t* eq, emergency_t *e);

// tira fuori l'emergenza più urgente
emergency_t* dequeue_emergency(emergency_queue_t* eq);

// ritorna la lista richiesta contenuta nella coda
emergency_list_t* get_list_by_priority(emergency_queue_t* eq, short priority);

void change_node_priority_list(emergency_queue_t* eq, emergency_node_t* node, short new_priority);

void promote_node_priority(emergency_queue_t* eq, emergency_node_t* node); // solo da 0 a 1 e non diversamente

// un nodo è das promuovere da 0 a 1 quando 
int is_node_to_promote(emergency_node_t* node);

#endif