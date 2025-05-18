#include "utils.h"
#include "parsers.h"
#include "debug.h"
#include <stdio.h>
#include <stdlib.h>

char* parse_env(int *height, int *width){

	// Apro il file di configurazione
	FILE *env_conf = fopen(ENV_CONF, "r");
	if(!env_conf) { 
		perror("Errore apertura file di configurazione " ENV_CONF);
		exit(EXIT_FAILURE);
	}

	char *line = NULL;
	size_t len = 0;

	char* queue = (char *)malloc((QUEUE_LENGTH_MINUS_ONE + 1) * sizeof(char));
	if(!queue) {
		perror("Errore allocazione memoria queue");
		exit(EXIT_FAILURE);
	}
	
	// estraggo la PRIMA riga del file
	my_getline(&line, &len, env_conf);
	if(sscanf(line, "queue=%" STR(QUEUE_LENGTH_MINUS_ONE) "s", queue) != 1){
		perror("Errore di sintassi nella 1a riga del file di configurazione " ENV_CONF);
		exit(EXIT_FAILURE);
	}

	queue[QUEUE_LENGTH_MINUS_ONE] = '\0'; // aggiungo il terminatore di stringa
	
	// estraggo la SECONDA riga del file
	my_getline(&line, &len, env_conf);
	if(sscanf(line, "height=%d", height) != 1){
		perror("Errore di sintassi nella 2a riga del file di configurazione " ENV_CONF);
		exit(EXIT_FAILURE);
	}

	// estraggo la TERZA riga del file
	my_getline(&line, &len, env_conf);
	if(sscanf(line, "width=%d", width) != 1){
		perror("Errore di sintassi nella 3a riga del file di configurazione " ENV_CONF);
		exit(EXIT_FAILURE);
	}

	// Controllo che i valori siano validi

	if(
		*height < MIN_COORDINATE || *height > MAX_COORDINATE ||
		*width < MIN_COORDINATE || *width > MAX_COORDINATE
	){
		perror("Valori illegali file di configurazione " ENV_CONF);
		exit(EXIT_FAILURE);
	}

	free(line);
	fclose(env_conf);
	return queue; // restituisco il nome della coda
}


void my_getline(char **line, size_t *len, FILE *stream){
	// Leggo la riga del file
	if(getline(line, len, stream) == -1){
		perror("Errore lettura file di configurazione" ENV_CONF);
		exit(EXIT_FAILURE);
	}
	
	// Controllo che il numero massimo di linee non venga superato
	if(strlen(*line) > MAX_LINE_LENGTH){
		perror("Riga troppo lunga file di configurazione " ENV_CONF);
		exit(EXIT_FAILURE);
	}

	// Controllo che la riga non sia vuota
	if((*line)[0] == '\n'){
		perror("Riga vuota file di configurazione " ENV_CONF);
		exit(EXIT_FAILURE);
	}
}
