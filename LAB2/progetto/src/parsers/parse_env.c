#include "parsers.h"

void parse_env(int *height, int *width){

	// Apro il file di configurazione
	FILE *env_conf = fopen(ENV_CONF, "r");
	check_opened_file_error_log(env_conf);

	char *line = NULL;
	size_t len = 0;

	char* queue_name = (char *)malloc((EMERGENCY_QUEUE_NAME_LENGTH + 1) * sizeof(char));
	check_error_memory_allocation(queue_name);
	
	// estraggo la PRIMA riga del file
	my_getline(&line, &len, env_conf);
	if(sscanf(line, "queue=%" STR(EMERGENCY_QUEUE_NAME_LENGTH) "s", queue_name) != 1)
		log_fatal_error("Errore di sintassi nella 1a riga del file di configurazione " ENV_CONF, FATAL_ERROR_PARSING);

	// estraggo la SECONDA riga del file
	my_getline(&line, &len, env_conf);
	if(sscanf(line, "height=%d", height) != 1)
		log_fatal_error("Errore di sintassi nella 2a riga del file di configurazione " ENV_CONF, FATAL_ERROR_PARSING);

	// estraggo la TERZA riga del file
	my_getline(&line, &len, env_conf);
	if(sscanf(line, "width=%d", width) != 1)
		log_fatal_error("Errore di sintassi nella 3a riga del file di configurazione " ENV_CONF, FATAL_ERROR_PARSING);

	// Controllo che i valori siano validi
	if(environment_values_are_illegal(queue_name, *height, *width))
		log_fatal_error("Valori illegali nel file di configurazione " ENV_CONF, FATAL_ERROR_PARSING);

	free(line);
	free(queue_name);
	fclose(env_conf);
}


void my_getline(char **line, size_t *len, FILE *stream){
	// Leggo la riga del file
	if(getline(line, len, stream) == -1)
		log_fatal_error("Errore di lettura nel file di configurazione " ENV_CONF, FATAL_ERROR_PARSING);
	
	// Controllo che il numero massimo di linee non venga superato
	if(strlen(*line) > MAX_LINE_LENGTH)
		log_fatal_error("Riga troppo lunga nel file di configurazione " ENV_CONF, FATAL_ERROR_PARSING);

	// Controllo che la riga non sia vuota
	if((*line)[0] == '\n')
		log_fatal_error("Riga vuota non ignorabile nel file di configurazione " ENV_CONF, FATAL_ERROR_PARSING);
}

int environment_values_are_illegal(char *queue, int h, int w){
	return (
		strcmp(queue, EMERGENCY_QUEUE_NAME) != 0 ||
		ABS(h) < MIN_Y_COORDINATE_ABSOLUTE_VALUE || 
		ABS(h) > MAX_Y_COORDINATE_ABSOLUTE_VALUE || 
		ABS(w) < MIN_X_COORDINATE_ABSOLUTE_VALUE || 
		ABS(w) > MAX_X_COORDINATE_ABSOLUTE_VALUE
	);
}