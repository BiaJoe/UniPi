#include "parsers.h"

// conto dei gemelli digitali, necessario per il logging
static int rescuer_digital_twins_total_count = 0;

void parse_rescuers(server_context_t *ctx){

	// Apro il file di configurazione
	FILE *rescuers_conf = fopen(RESCUERS_CONF, "r");
	check_opened_file_error_log(rescuers_conf);

	// inizializzo l'array di rescuer_types dinamicamente a NULL
	rescuer_type_t **rescuer_types = callocate_rescuer_types();

	// variabili temporanee per i campi del rescuer
	char name[MAX_RESCUER_NAME_LENGTH]; 
	int amount, speed, x, y;						
	
	// contatori di linee e numero di rescuer
	int local_rescuer_count = 0;
	int line_count = 0;

	// Leggo ogni riga del file e processo le informazioni contenute
	char *line = NULL;
	size_t len = 0;

	while (getline(&line, &len, rescuers_conf) != -1) {
		line_count++;

		// Controllo che il numero massimo di linee non venga superato
		if(line_count > MAX_FILE_LINES)
			log_fatal_error("Numero massimo di linee superato nel file di configurazione " RESCUERS_CONF, FATAL_ERROR_PARSING);

		// Controllo che il numero massimo di rescuer non venga superato
		if(local_rescuer_count > MAX_RESCUER_TYPES_COUNT)
			log_fatal_error("Numero massimo di rescuer superato nel file di configurazione " RESCUERS_CONF, FATAL_ERROR_PARSING);

		if (line[0] == '\n') {
			log_event(line_count, EMPTY_CONF_LINE_IGNORED, "Riga vuota ignorata: " RESCUERS_CONF);
			continue;
		}

		// Controllo la lunghezza della riga
		if(strlen(line) > MAX_LINE_LENGTH)
			log_fatal_error("Riga troppo lunga nel file di configurazione " RESCUERS_CONF, FATAL_ERROR_PARSING);

		// Controllo che la sintassi sia corretta ed estraggo i valori
		// conosco già il numero di campi e il loro tipo
		// quindi una sscanf basta per estrarre i valori
		if(sscanf(line, RESCUERS_SYNTAX , name, &amount, &speed, &x, &y) != 5)
			log_fatal_error("Errore di sintassi nel file di configurazione " RESCUERS_CONF, FATAL_ERROR_PARSING);

		// Controllo che i valori siano validi
		if(rescuer_type_values_are_illegal(ctx, name, amount, speed, x, y))
			log_fatal_error("Valori illegali nel file di configurazione " RESCUERS_CONF, FATAL_ERROR_PARSING);

		// Controllo che non ci siano duplicati, se ci sono ignoro la riga
		if(get_rescuer_type_by_name(name, rescuer_types)){ 
			log_event(line_count, DUPLICATE_RESCUER_TYPE_IGNORED, "Nome rescuer già presente, non aggiunto: " RESCUERS_CONF);
			continue;
		}
		
		// i campi sono validi e il nome non è presente, alloco il rescuer nel primo posto libero
		mallocate_and_populate_rescuer_type(name, amount, speed, x, y, rescuer_types); 

		local_rescuer_count++;
		log_event(line_count, RESCUER_TYPE_PARSED, "Rescuer aggiunto: " RESCUERS_CONF);
		
	}

	free(line);
	fclose(rescuers_conf);
	ctx -> rescuer_types_count = local_rescuer_count;
	ctx -> rescuer_types = rescuer_types;
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

void mallocate_and_populate_rescuer_type(char *name, int amount, int speed, int x, int y, rescuer_type_t **rescuer_types){
	//raggiujgo il primo posto libero in rescuer_types
	int i = 0;
	while(rescuer_types[i] != NULL) i++;

	// allco il rescuer_type_t
	rescuer_types[i] = (rescuer_type_t *)malloc(sizeof(rescuer_type_t));
	check_error_memory_allocation(rescuer_types[i]);

	// alloco il nome del rescuer_type_t e lo copio
	rescuer_types[i]->rescuer_type_name = (char *)malloc((strlen(name) + 1) * sizeof(char));
	check_error_memory_allocation(rescuer_types[i]->rescuer_type_name);


	// copio il nome
	strcpy(rescuer_types[i]->rescuer_type_name, name);

	// popolo il resto dei campi
	rescuer_types[i]->amount = amount;
	rescuer_types[i]->speed = speed;
	rescuer_types[i]->x = x;
	rescuer_types[i]->y = y;


	// alloco i rescuer_digital_twin_t (all'inizio tutti a NULL)
	rescuer_types[i]->twins = (rescuer_digital_twin_t **)calloc(amount + 1, sizeof(rescuer_digital_twin_t*));
	check_error_memory_allocation(rescuer_types[i]->twins);

	// alloco ogni twin e popolo i suoi campi 
	for(int j = 0; j < amount; j++){
		rescuer_types[i]->twins[j] = (rescuer_digital_twin_t *)malloc(sizeof(rescuer_digital_twin_t));
		check_error_memory_allocation(rescuer_types[i]->twins[j]);

		rescuer_types[i]->twins[j]->id = j;
		rescuer_types[i]->twins[j]->x = x;
		rescuer_types[i]->twins[j]->y = y;
		rescuer_types[i]->twins[j]->rescuer = rescuer_types[i];
		rescuer_types[i]->twins[j]->status = IDLE;
		rescuer_types[i]->twins[j]->is_travelling = NO;
		rescuer_types[i]->twins[j]->x_destination = x;
		rescuer_types[i]->twins[j]->y_destination = y;

		rescuer_digital_twins_total_count++;
		log_event(rescuer_digital_twins_total_count, RESCUER_DIGITAL_TWIN_ADDED, "Gemello digitale aggiunto: " RESCUERS_CONF);
	}
}

void free_rescuer_types(rescuer_type_t **rescuer_types){
	for(int i = 0; rescuer_types[i] != NULL; i++){
		free(rescuer_types[i]->rescuer_type_name);					//libero il puntatore al nome 
		for(int j = 0; j < rescuer_types[i]->amount; j++)		// libero ogni gemello digitale
			free(rescuer_types[i]->twins[j]);
		free(rescuer_types[i]->twins);											// libero l'array di puntatori ai gemelli digitali		
		free(rescuer_types[i]);															// libero il puntatore al rescuer_type_t 
	}	
	free(rescuer_types);																	// libero l'array di puntatori ai rescuer_types
}