#ifndef DEBUG_H
#define DEBUG_H

#include <stdio.h>
#include <stdlib.h>
#include "parsers.h"
#include "utils.h"


void print_rescuers(rescuer_type_t** rescuers, int rescuer_count);
char * get_status_string(rescuer_digital_twin_t *rescuer_digital_twin);
void print_emergency_types(emergency_type_t** emergency_types, int emergency_count);

#endif