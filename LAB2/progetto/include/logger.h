#ifndef LOGGER_H
#define LOGGER_H

#include <mqueue.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "structs.h"

#define LOG_QUEUE_NAME "/log_queue"
#define MAX_LOG_EVENT_LENGTH 512
#define MAX_LOG_QUEUE_MESSAGES 10

#define NO_ID -1
#define FATAL 1
#define NOT_FATAL 0
#define LOG 1
#define DONT_LOG 0
#define STOP_LOGGING_MESSAGE "-stop"

#define I_HAVE_TO_CLOSE_THE_LOG(m) ((strcmp(m, STOP_LOGGING_MESSAGE) == 0) ? 1 : 0 )

log_event_info_t log_event_lookup_table[LOG_EVENT_TYPES_COUNT];

void log_init();
void log_close();

#endif