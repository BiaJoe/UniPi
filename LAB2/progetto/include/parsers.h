#ifndef PARSERS_H
#define PARSERS_H

typedef enum {
	IDLE,
	EN_ROUTE_TO_SCENE,
	ON_SCENE,
	RETURNING_TO_BASE
} rescuer_status_t;

typedef struct {
	char *rescuer_type_name;
	int amount;
	int speed;
	int x;
	int y;
	rescuer_digital_twin_t *twins;
} rescuer_type_t;

typedef struct {
	int id;
	int x;
	int y;
	rescuer_type_t *rescuer;
	rescuer_status_t status;
} rescuer_digital_twin_t;

rescuer_type_t * get_rescuer_type(char *name, int amount, int speed, int x, int y);
rescuer_digital_twin_t * get_rescuer_digital_twin(rescuer_type_t * rescuer);
rescuer_type_t ** parse_rescuers();



#endif