#include "parsers.h"

static int rescuer_digital_twins_total_count = 0;								// conto dei gemelli digitali, necessario per il logging

void parse_rescuers(server_context_t *ctx){
	FILE *rescuers_conf = fopen(RESCUERS_CONF, "r");							// Apro il file di configurazione
	check_opened_file_error_log(rescuers_conf);
	rescuer_type_t **rescuer_types = callocate_rescuer_types();		// alloco i rescuer types tutti a NULL
	
	char filename = RESCUERS_CONF;
	char name[MAX_RESCUER_NAME_LENGTH]; 													// variabili temporanee per i campi del rescuer
	int amount, speed, x, y;						
	int rescuer_types_parsed_so_far = 0;																	// counters
	int line_count = 0;
	
	char *line = NULL;
	size_t len = 0;
	while (getline(&line, &len, rescuers_conf) != -1) {		// Leggo ogni riga del file e processo le informazioni contenute
		line_count++;		
		log_and_fail_if_file_line_cant_be_processed(line, line_count, MAX_RESCUER_CONF_LINES, rescuer_types_parsed_so_far, MAX_RESCUER_TYPES_COUNT, MAX_RESCUER_CONF_LINE_LENGTH, filename);
		if(check_and_log_if_line_is_empty(line, filename)) 
			continue;
		
		check_and_extract_rescuer_type_fields_from_line(ctx, filename, line_count, line, name, &amount, &speed, &x, &y);
		if(check_and_log_if_rescuer_type_already_parsed(filename, line_count, rescuer_types, name)) 
			continue;
		
		mallocate_and_populate_rescuer_type(name, amount, speed, x, y, rescuer_types, rescuer_types_parsed_so_far); 							// i campi sono validi e il nome non è presente, alloco il rescuer 
		log_event(line_count, RESCUER_TYPE_PARSED, "Rescuer %s con base in (%d, %d) e %d gemelli digitali aggiunto!", name, x, y, amount);
		rescuer_types_parsed_so_far++;
	}

	free(line);
	fclose(rescuers_conf);
	ctx -> rescuer_types_count = rescuer_types_parsed_so_far;
	ctx -> rescuer_types = rescuer_types;
}

int check_and_log_if_line_is_empty(char *line, char *filename){
	if (is_line_empty(line)){
		log_event(AUTOMATIC_LOG_ID, EMPTY_CONF_LINE_IGNORED, "linea vuota ignorata in %s", filename);
		return YES;
	}
	return NO;
}

int check_and_log_if_rescuer_type_already_parsed(char*filename, int line_count, rescuer_type_t **types, char* name){
	if(get_rescuer_type_by_name(name, types)){ 																																					// Controllo che non ci siano duplicati, se ci sono ignoro la riga
		log_event(line_count, DUPLICATE_RESCUER_TYPE_IGNORED, "Linea %d file %s: il rescuer %s è gìa stato aggiunto, ignorato", line_count, filename, name);
		return YES;
	}
	return NO;
}

void log_and_fail_if_file_line_cant_be_processed(char *line, int line_count, int max_lines, int lines_extracted_so_far, int max_extractable_lines, int max_line_length, char *filename){
	if(line_count > max_lines)												 	log_fatal_error("Numero massimo di linee superato in %s", filename);
	if(lines_extracted_so_far > max_extractable_lines)	log_fatal_error("Numero massimo di linee elaborate in %s", filename);
	if(strlen(line) > max_line_length)								 	log_fatal_error(LINE_FILE_ERROR_STRING "Linea troppo lunga", line_count, filename);
}


void check_and_extract_rescuer_type_fields_from_line(server_context_t *ctx, char* filename, int line_count, char *line, char *name, int *amount, int *speed, int *x, int *y){
	if(
		sscanf(line, RESCUERS_SYNTAX, name, amount, speed, x, y) != 5 ||																									
		rescuer_type_values_are_illegal(ctx, name, *amount, *speed, *x, *y)
	)																													
		log_fatal_error(LINE_FILE_ERROR_STRING "%s", line_count, filename, line);
}

int rescuer_type_values_are_illegal(server_context_t *ctx, char *name, int amount, int speed, int x, int y){
	return (
		strlen(name) <= 0 || 
		amount < MIN_RESCUER_AMOUNT || 
		amount > MAX_RESCUER_AMOUNT || 
		speed < MIN_RESCUER_SPEED || 
		speed > MAX_RESCUER_SPEED || 
		x < MIN_X_COORDINATE_ABSOLUTE_VALUE || 
		x > ctx -> width || 
		y < MIN_Y_COORDINATE_ABSOLUTE_VALUE || 
		y > ctx -> height
	);
}

rescuer_type_t ** callocate_rescuer_types(){
	rescuer_type_t **rescuer_types = (rescuer_type_t **)calloc((MAX_FILE_LINES + 1),  sizeof(rescuer_type_t*));
	check_error_memory_allocation(rescuer_types);
	return rescuer_types;
}

bresenham_trajectory_t *mallocate_bresenham_trajectory(){
	bresenham_trajectory_t *t = (bresenham_trajectory_t *)malloc(sizeof(bresenham_trajectory_t));
	check_error_memory_allocation(t);
	return t;
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
	t->trajectory			= NULL; 	// non inizializzato perchè nessun valore è sensato prima di aver dato una destinazione reale
	return t;
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

void mallocate_and_populate_rescuer_type(char *name, int amount, int speed, int x, int y, rescuer_type_t **rescuer_types, int i){
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
	rescuer_types[i] = r;
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