#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "utils.h"

#define LOG_FILE "log.txt"

#define LOG_QUEUE_NAME "/log_queue"
#define MAX_LOG_QUEUE_MESSAGES 10

#define AUTOMATIC_LOG_ID -1
#define NON_APPLICABLE_LOG_ID -2
#define NON_APPLICABLE_LOG_ID_STRING "N/A"
#define STOP_LOGGING_MESSAGE "-stop"

#define I_HAVE_TO_CLOSE_THE_LOG(m) ((strcmp(m, STOP_LOGGING_MESSAGE) == 0) ? 1 : 0 )

void logger(void);
void log_init(void);
void log_close(void);

#endif