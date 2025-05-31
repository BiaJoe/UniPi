#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <threads.h>

#include "logger.h"
#include "utils.h"

#define check_opened_file_error_log(fp) \
	do { \
		if(!fp) \
			log_fatal_error("errore apertura file:" #fp, FATAL_ERROR_FILE_OPENING); \
	}while(0);



// log.c serve per inviare messaggi di log a logger.c, che li scriverà su file

long get_time();
char* get_log_event_type_string(log_event_type_t event_type);
char* get_log_event_type_code(log_event_type_t event_type);


void log_event(int id, log_event_type_t event_type, char *message);
void log_fatal_error(char *message, log_event_type_t event);

void send_log_message(char message[LOG_EVENT_MESSAGE_LENGTH]);


#endif