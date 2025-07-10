#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h> 

#include "utils.h"

#define LOG_BUFFER_CAPACITY 32
#define HOW_MANY_LOGS_BEFORE_FLUSHING_THE_STREAM 8 

int compare_log_messages(const void *a, const void *b);
int we_should_flush(int logs_so_far);
void logger(void);

#endif