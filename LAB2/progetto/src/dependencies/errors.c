#include "errors.h"

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
		x < MIN_X_COORDINATE || 
		x > width || 
		y < MIN_Y_COORDINATE || 
		y > height
	);
}

int environment_values_are_illegal(int height, int width){
	return (
		height < MIN_Y_COORDINATE || 
		height > MAX_Y_COORDINATE || 
		width < MIN_X_COORDINATE || 
		width > MAX_X_COORDINATE
	);
}