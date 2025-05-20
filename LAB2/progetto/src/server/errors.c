#include "errors.h"

void log_fatal_error(char *message, log_event_type_t event) {
	log_event(0, event, message); // id = 0 perchè un errore fatale può accadere una sola volta
	log_close();
	exit(EXIT_FAILURE);
}

void check_opened_file(FILE *file, char *filename) {
	if (file) return;
	char* message = "Errore durante apertura del file: ";
	strcat(message, filename);
	log_fatal_error(message, FATAL_ERROR_FILE_OPENING); 
	exit(EXIT_FAILURE);
}

int rescuer_request_values_are_illegal(char *rr_name, int required_count, int time_to_manage){
	return (
		strlen(rr_name) <= 0 || 
		required_count < MIN_RESCUER_REQUIRED_COUNT || 
		required_count > MAX_RESCUER_REQUIRED_COUNT || 
		time_to_manage < MIN_TIME_TO_MANAGE || 
		time_to_manage > MAX_TIME_TO_MANAGE
	);
}

int emergency_values_are_illegal(char *emergency_desc, short priority){
	return (
		strlen(emergency_desc) <= 0 || 
		priority < MIN_EMERGENCY_PRIORITY || 
		priority > MAX_EMERGENCY_PRIORITY
	);
}

int rescuer_type_values_are_illegal(char *name, int amount, int speed, int x, int y){
	return (
		strlen(name) <= 0 || 
		amount < MIN_RESCUER_AMOUNT || 
		amount > MAX_RESCUER_AMOUNT || 
		speed < MIN_RESCUER_SPEED || 
		speed > MAX_RESCUER_SPEED || 
		x < MIN_COORDINATE || 
		x > MAX_COORDINATE || 
		y < MIN_COORDINATE || 
		y > MAX_COORDINATE 
	);
}

int environment_values_are_illegal(int height, int width){
	return (
		height < MIN_COORDINATE || 
		height > MAX_COORDINATE || 
		width < MIN_COORDINATE || 
		width > MAX_COORDINATE
	);
}