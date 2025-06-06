#ifndef COSTANTS_H
#define COSTANTS_H

#define RESCUERS_CONF        "conf/rescuers.conf"
#define EMERGENCY_TYPES_CONF "conf/emergency_types.conf"
#define ENV_CONF             "conf/env.conf"

#define MAX_FILE_LINES 1024
#define MAX_LINE_LENGTH 1024

#define MAX_RESCUER_NAME_LENGTH 128
#define EMERGENCY_NAME_LENGTH 64
#define MAX_RESCUER_REQUESTS_LENGTH 1024
#define MAX_RESCUER_TYPES_COUNT 64
#define MAX_EMERGENCY_TYPES_COUNT 128
#define MAX_RESCUER_REQ_NUMBER_PER_EMERGENCY 16
#define MAX_ENV_FIELD_LENGTH 32

// log
#define LOG_EVENT_NAME_LENGTH 64
#define LOG_EVENT_CODE_LENGTH 5
#define LOG_EVENT_MESSAGE_LENGTH 256

#define MIN_EMERGENCY_PRIORITY 0
#define MAX_EMERGENCY_PRIORITY 3
#define MIN_RESCUER_SPEED 1
#define MAX_RESCUER_SPEED 100
#define MIN_RESCUER_AMOUNT 1
#define MAX_RESCUER_AMOUNT 1000
#define MIN_RESCUER_REQUIRED_COUNT 1
#define MAX_RESCUER_REQUIRED_COUNT 32
#define MIN_X_COORDINATE 0
#define MAX_X_COORDINATE 1024
#define MIN_Y_COORDINATE 0
#define MAX_Y_COORDINATE 1024
#define MIN_TIME_TO_MANAGE 1
#define MAX_TIME_TO_MANAGE 1000

// estratte da env.conf
extern int height;
extern int width;

#define EMERGENCY_QUEUE_NAME_LENGTH 16
#define EMERGENCY_QUEUE_NAME "emergenze676722" // lunghezza 16
#define MAX_EMERGENCY_QUEUE_MESSAGE_LENGTH 512

#define LONG_LENGTH 20

// sintassi varie

#define LOG_EVENT_STRING_SYNTAX "%-15ld %s %-5d %-35s %s\n"
// #define LOG_EVENT_STRING_SYNTAX "[%ld] [%s%d] [%s] [%s]\n"
#define RESCUERS_SYNTAX "[%" STR(MAX_RESCUER_NAME_LENGTH) "[^]]][%d][%d][%d;%d]"
#define RESCUER_REQUEST_SYNTAX "%" STR(MAX_RESCUER_NAME_LENGTH) "[^:]:%d,%d"
#define EMERGENCY_TYPE_SYNTAX "[%" STR(EMERGENCY_NAME_LENGTH) "[^]]] [%d] %" STR(MAX_RESCUER_REQUESTS_LENGTH) "[^\n]"
#define EMERGENCY_REQUEST_SYNTAX "%" STR(EMERGENCY_NAME_LENGTH) "[^ ] %d %d %ld"
#define EMERGENCY_REQUEST_ARGUMENT_SEPARATOR " " // nel file si separano gli argomenti con lo spazio, questo impone che i nomi non contengano spazi tra l'altro

#endif