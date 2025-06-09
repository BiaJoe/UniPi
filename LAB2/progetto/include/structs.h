#ifndef STRUCTS_H
#define STRUCTS_H

#include <time.h>
#include <mqueue.h>
#include <threads.h>
#include <stdatomic.h>
#include "costants.h"


// STRUTTURE PER I RESCUER

typedef enum {
	IDLE,
	EN_ROUTE_TO_SCENE,
	ON_SCENE,
	RETURNING_TO_BASE
} rescuer_status_t;

// Foreward declaration per rescuer_digital_twin
// per evitare dipendenze circolari
struct rescuer_digital_twin; 
typedef struct rescuer_digital_twin rescuer_digital_twin_t;

typedef struct {
	char *rescuer_type_name;
	int amount;
	int speed;
	int x;
	int y;
	rescuer_digital_twin_t **twins;
} rescuer_type_t;

struct rescuer_digital_twin {
	int id;
	int x;
	int y;
	rescuer_type_t *rescuer;
	rescuer_status_t status;
};




// STRUTTURE PER LE EMERGENZE

typedef struct {
	rescuer_type_t *type;
	int required_count;
	int time_to_manage;
} rescuer_request_t;

typedef struct {
	short priority;
	char *emergency_desc;
	rescuer_request_t **rescuers;
	int rescuers_req_number;
} emergency_type_t;

typedef enum {
	WAITING,
	ASSIGNED,
	IN_PROGRESS,
	PAUSED,
	COMPLETED,
	CANCELED,
	TIMEOUT
} emergency_status_t;


// STRUTTURE PER LE RICHIESTE DI EMERGENZA

typedef struct {
	char emergency_name[EMERGENCY_NAME_LENGTH];
	int x;
	int y;
	time_t timestamp;
} emergency_request_t;


typedef struct {
	emergency_type_t *type;
	emergency_status_t status;
	int x;
	int y;
	time_t time;
	int rescuer_count;
	rescuer_digital_twin_t **rescuer_twins;
} emergency_t;

// STRUTTURE PER IL LOGGING

typedef struct {
	char name[LOG_EVENT_NAME_LENGTH];	// versione stringa del tipo
	char code[LOG_EVENT_CODE_LENGTH];	// versione codice del tipo
	atomic_int counter;								// quante volte è stato registrato
	int is_terminating;								// se loggarlo vuol dire terminare il programma		
	int is_to_log;										// se va scritto o no nel file di log
} log_event_info_t;

#define LOG_EVENT_TYPES_COUNT 27

typedef enum {

	NON_APPLICABLE, 								  //N/A
	
	// errori fatali
	FATAL_ERROR, 											//FERR
	FATAL_ERROR_PARSING, 							//FEPA
	FATAL_ERROR_LOGGING, 							//FELO
	FATAL_ERROR_MEMORY, 							//FEME
	FATAL_ERROR_FILE_OPENING,					//FEFO
	
	// errori non fatali
	EMPTY_CONF_LINE_IGNORED,					//ECLI		
	DUPLICATE_RESCUER_REQUEST_IGNORED,//DRRI
	DUPLICATE_EMERGENCY_TYPE_IGNORED, //DETI
	DUPLICATE_RESCUER_TYPE_IGNORED,		//DRTI
	WRONG_EMERGENCY_REQUEST_IGNORED_CLIENT, //WERC
	WRONG_EMERGENCY_REQUEST_IGNORED_SERVER, //WERS
	// eventi di log
	LOGGING_STARTED, 									//LSTA
	LOGGING_ENDED,  									//LEND

	// eventi di parsing
	PARSING_STARTED,									//PSTA
	PARSING_ENDED,										//PEND
	RESCUER_TYPE_PARSED,							//RTPA
	RESCUER_DIGITAL_TWIN_ADDED,				//RDTA
	EMERGENCY_PARSED,									//EMPA		
	RESCUER_REQUEST_ADDED,						//RRAD

	// eventi di gestione richieste emergenza
	EMERGENCY_REQUEST_RECEIVED, 			//ERRR
	EMERGENCY_REQUEST_PROCESSED,			//ERPR

	MESSAGE_QUEUE, 										//MQUE
	EMERGENCY_STATUS, 								//ESTA
	RESCUER_STATUS, 									//RSTA
	EMERGENCY_REQUEST,								//ERRE

	PROGRAM_ENDED_SUCCESSFULLY,				//PESU

	// ...aggiungere altri tipi di log qui
} log_event_type_t; 

// emergency queue

// forward declaration per node e list
struct emergency_list;
typedef struct emergency_list emergency_list_t;

typedef struct emergency_node{
	emergency_list_t *list;
	emergency_t *emergency;
	struct emergency_node *prev;
	struct emergency_node *next;
	mtx_t node_mutex;
} emergency_node_t;

typedef struct {
	emergency_node_t *head;
	emergency_node_t *tail;
	int node_amount; // inizia a 0 con head = tail = NULL
	mtx_t list_mutex;
} emergency_list_t;

typedef struct {
	emergency_list_t* queue[PRIORITY_LEVELS]; // arrays di puntatori, un per ogni priorità
	emergency_list_t* finished;
} emergency_queue_t;

// server

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



#endif