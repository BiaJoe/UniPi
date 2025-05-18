#include "utils.h"


int main(){
	
	// variabili inizializzate per il parsing
	int rescuer_count = 0;
	int emergency_count = 0;
	int height = 0;
	int width = 0;
	char* queue = NULL;
	rescuer_type_t** rescuers = NULL;
	emergency_type_t** emergency_types = NULL;

	// Parsing dei file di configurazione
	queue = parse_env(&height, &width);
	rescuers = parse_rescuers(&rescuer_count);
	emergency_types = parse_emergencies(&emergency_count, rescuers);

	// Stampa delle informazioni
	print_env(queue, height, width);
	print_rescuer_types(rescuers, rescuer_count);
	print_emergency_types(emergency_types, emergency_count);

	// Libero la memoria
	free_rescuer_types(rescuers);
	free_emergency_types(emergency_types);
	return 0;
}