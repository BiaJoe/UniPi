#include <time.h>

#ifndef PARSERS_H
#define PARSERS_H

#define RESCUERS_CONF "conf/rescuers.conf"
#define EMERGENCY_TYPES_CONF "conf/emergency_types.conf"
#define ENV_CONF "conf/env.conf"

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

#define MAX_FILE_LINES 512
#define MAX_LINE_LENGTH 1024

#define MAX_RESCUER_NAME_LENGTH 128
#define EMERGENCY_NAME_LENGTH 64
#define MAX_RESCUER_REQUESTS_LENGTH 1024
#define MAX_RESCUER_COUNT 64
#define MAX_EMERGENCY_COUNT 128
#define MAX_RESCUER_REQ_NUMBER_PER_EMERGENCY 16

#define MIN_EMERGENCY_PRIORITY 0
#define MAX_EMERGENCY_PRIORITY 3
#define MIN_RESCUER_SPEED 1
#define MAX_RESCUER_SPEED 100

//ancora non usate
#define MIN_RESCUER_AMOUNT 1
#define MAX_RESCUER_AMOUNT 1000
#define MIN_COORDINATE 0
#define MAX_COORDINATE 1000
#define MIN_TIME_TO_MANAGE 1
#define MAX_TIME_TO_MANAGE 1000
#define MAX_EMERGENCY_TYPES 64

#define RESCUERS_SYNTAX "[%" STR(MAX_RESCUER_NAME_LENGTH) "[^]]][%d][%d][%d;%d]"
#define RESCUER_REQUEST_SYNTAX "%" STR(MAX_RESCUER_NAME_LENGTH) "[^:]:%d,%d"
#define EMERGENCY_TYPE_SYNTAX "[%" STR(EMERGENCY_NAME_LENGTH) "[^]]][%d] %" STR(MAX_RESCUER_REQUESTS_LENGTH) "[^\n]"

#define IGNORE_EMPITY_LINES() if (line[0] == '\n') continue




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






typedef struct {
    struct rescuer_type_t *type;
    int required_count;
    int time_to_manage;
} rescuer_request_t;

typedef struct {
    short priority;
    char *emergency_desc;
    rescuer_request_t *rescuers;
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






int rescuer_arleady_exists(char *name, char rescuer_names_buffer[][MAX_RESCUER_NAME_LENGTH]);
rescuer_type_t ** parse_rescuers(int* rescuer_types);

rescuer_type_t ** parse_rescuers2(int* rescuer_types);
rescuer_type_t ** init_rescuer_types();
int rescuer_values_are_illegal(char *name, int amount, int speed, int x, int y);
void allocate_rescuer_type(char *name, int amount, int speed, int x, int y, rescuer_type_t **rescuer_types);
rescuer_type_t * get_rescuer_type_by_name(char *name, rescuer_type_t **rescuer_types);
int get_rescuer_type_index_by_name(char *name, rescuer_type_t **rescuer_types);

emergency_type_t ** parse_emergencies(int* emergency_types);

void check_emergency_type_syntax_and_extract_values(
    char *line, 
    short *priority, 
    char *emergency_desc, 
    char rescuer_names_buffer[][MAX_RESCUER_NAME_LENGTH],
    rescuer_request_t *rescuer_requests_buffer, 
    int *rescuer_req_number,
    char emergency_names_buffer[][EMERGENCY_NAME_LENGTH]
);

#endif