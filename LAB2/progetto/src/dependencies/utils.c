#include "utils.h"

int my_atoi(char a[]){
	int order = strlen(a) - 1;
	int res = 0;
	char c;
	int i = 0;
	int is_negative = 0;

	if((a[0]) == '-'){ // numero negativo o positivo?
		i = 1;
		is_negative = 1;
		order--;
	}

	if((a[i]) == '0'){ 	// controllo che la prima cifra (i = 0 o i = 1) non sia zero
		errno = EINVAL;
		return 0;
	}

	while((c = a[i]) != '\0'){
		if(c < '0' || c > '9'){												// controllo che la cifra sia numerica
			errno = EINVAL;
			return 0;
		}
		res += (c - '0') * ((int)pow(10, order--));		// aggiorno il risultato
	}

	if(is_negative) 
		return -res;
	
	return res;
}

void write_line(FILE *f, char *s) {
	check_error_fopen(f);
	fprintf(f, "%s", s);
	fflush(f);
}

int is_line_empty(char *line){
	if (line == NULL) return 1;
	while(*line) if(!isspace(*(line++))) return 0;
	return 1;
}


rescuer_type_t * get_rescuer_type_by_name(char *name, rescuer_type_t **rescuer_types){
	for(int i = 0; rescuer_types[i] != NULL; i++){
		if(strcmp(rescuer_types[i]->rescuer_type_name, name) == 0){
			return rescuer_types[i];
		}
	}
	return NULL;
}

char* get_name_of_rescuer_requested(rescuer_request_t *rescuer_request){
	rescuer_type_t *rescuer_type = (rescuer_type_t *)rescuer_request->type;
	return rescuer_type->rescuer_type_name;
}

emergency_type_t * get_emergency_type_by_name(char *name, emergency_type_t **emergency_types){
	for(int i = 0; emergency_types[i] != NULL; i++)
		if(strcmp(emergency_types[i]->emergency_desc, name) == 0)
			return emergency_types[i];
	return NULL;
}

rescuer_request_t * get_rescuer_request_by_name(char *name, rescuer_request_t **rescuers){
	for(int i = 0; rescuers[i] != NULL; i++)
		if(strcmp(get_name_of_rescuer_requested(rescuers[i]), name) == 0)
			return rescuers[i];
	return NULL;
}