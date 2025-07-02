#include "parsers.h"

static int rescuer_digital_twins_total_count = 0;								// conto dei gemelli digitali, necessario per il logging

void parse_rescuers(server_context_t *ctx){
	parsing_state_t *ps = mallocate_parsing_state(RESCUERS_CONF);
	rescuer_type_t **rescuer_types = callocate_rescuer_types();		// alloco i rescuer types tutti a NULL
	char name[MAX_RESCUER_NAME_LENGTH]; 													// variabili temporanee per i campi del rescuer
	int amount, speed, x, y;						
	
	while (go_to_next_line(ps)) {		// Leggo ogni riga del file e processo le informazioni contenute
		log_and_fail_if_file_line_cant_be_processed(ps, MAX_RESCUER_CONF_LINES, MAX_RESCUER_TYPES_COUNT, MAX_RESCUER_CONF_LINE_LENGTH);
		if(check_and_log_if_line_is_empty(ps)) 
			continue;
		check_and_extract_rescuer_type_fields_from_line(ps, ctx->width, ctx->height, name, &amount, &speed, &x, &y);
		if(check_and_log_if_rescuer_type_already_parsed(ps, rescuer_types, name)) 
			continue;
		rescuer_types[ps->parsed_so_far] = mallocate_and_populate_rescuer_type(name, amount, speed, x, y); 							// i campi sono validi e il nome non è presente, alloco il rescuer 
		log_event(ps->line_number, RESCUER_TYPE_PARSED, "Rescuer %s con base in (%d, %d) e %d gemelli digitali aggiunto!", name, x, y, amount);
		ps->parsed_so_far++;
	}
	ctx -> rescuer_types_count = ps->parsed_so_far;
	ctx -> rescuer_types = rescuer_types;
	free_parsing_state(ps);
}

void check_and_extract_rescuer_type_fields_from_line(parsing_state_t *ps, int maxx, int maxy, char *name, int *amount, int *speed, int *x, int *y){
	if(
		sscanf(ps->line, RESCUERS_SYNTAX, name, amount, speed, x, y) != 5 ||																									
		rescuer_type_values_are_illegal(name, *amount, *speed, *x, *y, maxx, maxy)
	)																													
		log_fatal_error(LINE_FILE_ERROR_STRING "%s", ps->line_number, ps->filename, ps->line);
}

int check_and_log_if_rescuer_type_already_parsed(parsing_state_t *ps, rescuer_type_t **types, char* name){
	if(get_rescuer_type_by_name(name, types)){ 																																					// Controllo che non ci siano duplicati, se ci sono ignoro la riga
		log_event(ps->line_number, DUPLICATE_RESCUER_TYPE_IGNORED, "Linea %d file %s: il rescuer %s è gìa stato aggiunto, ignorato", ps->line_number, ps->filename, name);
		return YES;
	}
	return NO;
}

int rescuer_type_values_are_illegal(char *name, int amount, int speed, int x, int y, int maxx, int maxy){
	return (
		strlen(name) <= 0 || 
		amount < MIN_RESCUER_AMOUNT || 
		amount > MAX_RESCUER_AMOUNT || 
		speed < MIN_RESCUER_SPEED || 
		speed > MAX_RESCUER_SPEED || 
		x < MIN_X_COORDINATE_ABSOLUTE_VALUE || 
		x > maxx || 
		y < MIN_Y_COORDINATE_ABSOLUTE_VALUE || 
		y > maxy
	);
}

rescuer_type_t ** callocate_rescuer_types(){
	rescuer_type_t **rescuer_types = (rescuer_type_t **)calloc((MAX_FILE_LINES + 1),  sizeof(rescuer_type_t*));
	check_error_memory_allocation(rescuer_types);
	return rescuer_types;
}

rescuer_type_t *mallocate_and_populate_rescuer_type(char *name, int amount, int speed, int x, int y){
	rescuer_type_t *r = (rescuer_type_t *)malloc(sizeof(rescuer_type_t));
	check_error_memory_allocation(r);
	r->rescuer_type_name = (char *)malloc((strlen(name) + 1) * sizeof(char));	// alloco il nome del rescuer_type_t e lo copio
	check_error_memory_allocation(r->rescuer_type_name);
	strcpy(r->rescuer_type_name, name);																				// popolo i campi del rescuer type
	r->amount = amount;									
	r->speed = speed;										
	r->x = x;											
	r->y = y;
	r->twins = callocate_and_populate_rescuer_digital_twins(r);
	return r;
}

rescuer_digital_twin_t **callocate_and_populate_rescuer_digital_twins(rescuer_type_t* r){
	rescuer_digital_twin_t **twins = (rescuer_digital_twin_t **)calloc(r->amount + 1, sizeof(rescuer_digital_twin_t*));
	check_error_memory_allocation(twins);
	for(int i = 0; i < r->amount; i++){
		twins[i] = mallocate_rescuer_digital_twin(r);
		log_event(twins[i]->id, RESCUER_DIGITAL_TWIN_ADDED, "%s #%d [%d, %d] Aggiunto ai gemelli digitali ", r->rescuer_type_name, twins[i]->id, twins[i]->x, twins[i]->y);
	}
	return twins;
}

rescuer_digital_twin_t *mallocate_rescuer_digital_twin(rescuer_type_t* r){
	rescuer_digital_twin_t *t = (rescuer_digital_twin_t *)malloc(sizeof(rescuer_digital_twin_t));
	check_error_memory_allocation(t);
	t->id 						= rescuer_digital_twins_total_count++;			// in questo modo ogni gemello è unico
	t->x 							= r->x;
	t->y 							= r->y;
	t->rescuer 				= r;
	t->status 				= IDLE;
	t->is_travelling 	= NO;
	t->emergency 			= NULL; 
	t->trajectory			= mallocate_bresenham_trajectory(); 	// non inizializzato perchè nessun valore è sensato prima di aver dato una destinazione reale
	return t;
}

rescuer_digital_twin_t *mallocate_rescuer_digital_twin(rescuer_type_t* r){
	bresenham_trajectory_t *t = (bresenham_trajectory_t *)malloc(sizeof(bresenham_trajectory_t));
	check_error_memory_allocation(t);
	return t;
}

void free_rescuer_types(rescuer_type_t **rescuer_types){
	for(int i = 0; rescuer_types[i] != NULL; i++){
		free(rescuer_types[i]->rescuer_type_name);					//libero il puntatore al nome 
		for(int j = 0; j < rescuer_types[i]->amount; j++){	// libero ogni gemello digitale
			free(rescuer_types[i]->twins[j]->trajectory);			// libero la struttura usata per gestire i suoi movimenti
			free(rescuer_types[i]->twins[j]);
		}
		free(rescuer_types[i]->twins);											// libero l'array di puntatori ai gemelli digitali		
		free(rescuer_types[i]);															// libero il puntatore al rescuer_type_t 
	}	
	free(rescuer_types);																	// libero l'array di puntatori ai rescuer_types
}