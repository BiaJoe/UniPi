#ifndef ERRORS_H
#define ERRORS_H

#include "log.h"

#define CHECK_FOR_MEMORY_ALLOCATION_ERROR(pointer) \
	do { \
		if (!(pointer)) \
			log_fatal_error("Errore allocazione memoria: " #pointer, FATAL_ERROR_MEMORY); \
	} while (0)

void log_fatal_error(char *message, log_event_type_t event);
void check_opened_file(FILE *file, char *filename);

int rescuer_type_values_are_illegal(char *name, int amount, int speed, int x, int y);
int rescuer_request_values_are_illegal(char *rr_name, int required_count, int time_to_manage);
int emergency_values_are_illegal(char *emergency_desc, short priority);
int environment_values_are_illegal(int height, int width);

#endif