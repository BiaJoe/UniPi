#ifndef STRUCTS_H
#define STRUCTS_H

#include <time.h>
#include <mqueue.h>
#include <threads.h>
#include <stdatomic.h>
#include "costants.h"


// STRUTTURE PER IL LOGGING

typedef struct {
	char name[LOG_EVENT_NAME_LENGTH];	// versione stringa del tipo
	char code[LOG_EVENT_CODE_LENGTH];	// versione codice del tipo
	atomic_int counter;								// quante volte è stato registrato
	int is_terminating;								// se loggarlo vuol dire terminare il programma		
	int is_to_log;										// se va scritto o no nel file di log
} log_event_info_t;

#define LOG_EVENT_TYPES_COUNT 32

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
	WRONG_RESCUER_REQUEST_IGNORED, 		//WRRI
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

	SERVER, //srvr
	CLIENT, //clnt

	// eventi di gestione richieste emergenza
	EMERGENCY_REQUEST_RECEIVED, 			//ERRR
	EMERGENCY_REQUEST_PROCESSED,			//ERPR

	MESSAGE_QUEUE_CLIENT, 						//MQCL
	MESSAGE_QUEUE_SERVER, 						//MQSE

	EMERGENCY_STATUS, 								//ESTA
	RESCUER_STATUS, 									//RSTA
	RESCUER_TRAVELLING_STATUS,         //RTST
	EMERGENCY_REQUEST,								//ERRE

	PROGRAM_ENDED_SUCCESSFULLY,				//PESU

	// ...aggiungere altri tipi di log qui
} log_event_type_t; 


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
	int is_travelling;
	int x_destination;
	int y_destination;
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

// emergency queue

// forward declaration per node e list
struct emergency_list;
typedef struct emergency_list emergency_list_t;

typedef struct emergency_node{
	emergency_list_t *list;
	emergency_t *emergency;
	struct emergency_node *prev;
	struct emergency_node *next;
} emergency_node_t;

typedef struct {
	emergency_node_t *head;
	emergency_node_t *tail;
	int node_amount; 
	mtx_t mutex;
} emergency_list_t;

typedef struct {
	emergency_list_t* lists[PRIORITY_LEVELS]; 
	mtx_t mutex;
} emergency_queue_t;

// server

typedef struct {
	int height;														// interi per tenere traccia di cosa succede
	int width;
	int rescuer_types_count;
	int emergency_types_count;
	int emergency_requests_count;
	rescuer_type_t** rescuer_types;				// puntatori alle strutture rescuers
	mtx_t rescuers_mutex;									// mutex per proteggere l'accesso ai rescuer types
  emergency_type_t** emergency_types;		// puntatori alle strutture emergency_types
	emergency_queue_t* queue;							// coda per contenere le emergenze da processare
	mqd_t mq;															// message queue per ricevere le emergenze dai client	
	time_t current_time;									// tempo corrente del server
	int tick;															// tick del server, per sincronizzare i thread
	int tick_count_since_start;						// contatore dei tick del server, per tenere traccia di quanti tick sono stati fatti
	mtx_t clock_mutex;										// mutex per proteggere l'accesso al tick del server
	cnd_t clock_updated;							// condizione per comunicare al therad updater di fare l'update
	

} server_context_t;



#endif