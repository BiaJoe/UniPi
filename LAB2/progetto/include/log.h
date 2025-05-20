#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <threads.h>

#include "utils.h"


long get_time();
void log_init();
char *get_log_event_type_string(log_event_type_t event_type);
char* get_log_event_type_code(log_event_type_t event_type);
void log_event(int id, log_event_type_t event_type, char *message);
void log_close();

#endif