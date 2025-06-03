#ifndef LOGGER_H
#define LOGGER_H

#include <mqueue.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "utils.h"

#define LOG_FILE "log.txt"

#define LOG_QUEUE_NAME "/log_queue"
#define MAX_LOG_EVENT_LENGTH 512
#define MAX_LOG_QUEUE_MESSAGES 10

#define NO_ID -1
#define STOP_LOGGING_MESSAGE "-stop"

#define I_HAVE_TO_CLOSE_THE_LOG(m) ((strcmp(m, STOP_LOGGING_MESSAGE) == 0) ? 1 : 0 )

void logger(void);
void log_init(void);
void log_close(void);

#endif