#include "parsers.h"

rescuer_type_t** parse_rescuers(int *rescuer_count){

	// Apro il file di configurazione
	FILE *rescuers_conf = fopen(RESCUERS_CONF, "r");
	check_opened_file_error_log(rescuers_conf);

	// inizializzo l'array di rescuer_types dinamicamente a NULL
	rescuer_type_t **  rescuer_types = callocate_rescuer_types();

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
		if(rescuer_type_values_are_illegal(name, amount, speed, x, y))
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
	*rescuer_count = local_rescuer_count;

	return rescuer_types;
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

