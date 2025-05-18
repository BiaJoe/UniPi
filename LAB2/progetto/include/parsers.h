#ifndef PARSERS_H
#define PARSERS_H

#include <time.h>
#include <stdio.h>
#include <stdlib.h>

#define RESCUERS_CONF        "conf/rescuers.conf"
#define EMERGENCY_TYPES_CONF "conf/emergency_types.conf"
#define ENV_CONF             "conf/env.conf"

#define MAX_FILE_LINES 512
#define MAX_LINE_LENGTH 1024

#define MAX_RESCUER_NAME_LENGTH 128
#define EMERGENCY_NAME_LENGTH 64
#define MAX_RESCUER_REQUESTS_LENGTH 1024
#define MAX_RESCUER_TYPES_COUNT 64
#define MAX_EMERGENCY_TYPES_COUNT 128
#define MAX_RESCUER_REQ_NUMBER_PER_EMERGENCY 16
#define MAX_ENV_FIELD_LENGTH 32

#define MIN_EMERGENCY_PRIORITY 0
#define MAX_EMERGENCY_PRIORITY 3
#define MIN_RESCUER_SPEED 1
#define MAX_RESCUER_SPEED 100
#define MIN_RESCUER_AMOUNT 1
#define MAX_RESCUER_AMOUNT 1000
#define MIN_RESCUER_REQUIRED_COUNT 1
#define MAX_RESCUER_REQUIRED_COUNT 32
#define MIN_COORDINATE 0
#define MAX_COORDINATE 1000
#define MIN_TIME_TO_MANAGE 1
#define MAX_TIME_TO_MANAGE 1000

#define QUEUE_LENGTH_MINUS_ONE 15
#define QUEUE "emergenze676722" // lunghezza 16

#define RESCUERS_SYNTAX "[%" STR(MAX_RESCUER_NAME_LENGTH) "[^]]][%d][%d][%d;%d]"
#define RESCUER_REQUEST_SYNTAX "%" STR(MAX_RESCUER_NAME_LENGTH) "[^:]:%d,%d"
#define EMERGENCY_TYPE_SYNTAX "[%" STR(EMERGENCY_NAME_LENGTH) "[^]]] [%d] %" STR(MAX_RESCUER_REQUESTS_LENGTH) "[^\n]"
#define ENV_SYNTAX ""

#define IGNORE_EMPITY_LINES() if (line[0] == '\n') continue



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
    emergency_type_t type;
    emergency_status_t status;
    int x;
    int y;
    time_t time;
    int rescuer_count;
    rescuer_digital_twin_t *rescuers_dt;
} emergency_t;




// Funzioni per la gestione dei file di configurazione

// Funzioni per la gestione dei rescuer
rescuer_type_t ** parse_rescuers(int* rescuer_types);
int rescuer_values_are_illegal(char *name, int amount, int speed, int x, int y);


// Funzioni per la gestione delle emergenze
emergency_type_t ** parse_emergencies(int* emergency_count, rescuer_type_t **rescuer_types);
rescuer_request_t ** init_resquer_requests();
void check_emergency_type_syntax_and_extract_values(
	char *line, 
	short *priority, 
	char *emergency_desc, 
	rescuer_request_t **rescuers,
	int *rescuer_req_number,
	rescuer_type_t **rescuer_types
);
int emergency_values_are_illegal(char *emergency_desc, short priority);
int rescuer_request_values_are_illegal(char *rr_name, int required_count, int time_to_manage);
void allocate_emergency_type(
    short priority, 
    char *emergency_desc, 
    int rescuer_req_number,
    rescuer_request_t **rescuers,
    emergency_type_t **emergency_types
);
void allocate_rescuer_request(
	char *rr_name, 
	int required_count, 
	int time_to_manage, 
	rescuer_request_t **rescuers,
	rescuer_type_t **rescuer_types
);

// Funzioni per la gestione dell'ambiente
char* parse_env(int *height, int *width);
void my_getline(char **line, size_t *len, FILE *stream);


#endif