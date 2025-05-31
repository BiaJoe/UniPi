#include "server.h"


int height;
int width;

int main(){

	// variabili inizializzate per il parsing
	int rescuer_count = 0;
	int emergency_count = 0;
	char* queue = NULL;
	rescuer_type_t** rescuers = NULL;
	emergency_type_t** emergency_types = NULL;


	// Parsing dei file di configurazione
	log_event(0, PARSING_STARTED, "Inizio parsing dei file di configurazione");

	queue = parse_env(&height, &width);
	rescuers = parse_rescuers(&rescuer_count);
	emergency_types = parse_emergencies(&emergency_count, rescuers);

	log_event(0, PARSING_ENDED, "Il parsing è terminato con successo!");

	// Stampa delle informazioni
	print_env(queue, height, width);
	print_rescuer_types(rescuers, rescuer_count);
	print_emergency_types(emergency_types, emergency_count);

	// Libero la memoria
	free_rescuer_types(rescuers);
	free_emergency_types(emergency_types);
	free(queue);


	return 0;
}