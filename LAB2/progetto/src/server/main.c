#include <stdio.h>
#include <stdlib.h> 
#include "utils.h"


int main(){
	int rescuer_count = 0, emergency_count = 0;
	rescuer_type_t** rescuers =	parse_rescuers(&rescuer_count);
	emergency_type_t** emergency_types = parse_emergencies(&emergency_count, rescuers);

	print_rescuers(rescuers, rescuer_count);
	print_emergency_types(emergency_types, emergency_count);

	return 0;
}