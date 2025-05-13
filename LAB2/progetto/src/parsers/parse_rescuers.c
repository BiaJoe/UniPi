#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
// #include <ctype.h>
#include "../include/parsers.h"

rescuer_type_t** parse_rescuers(int *rescuer_count){

	// Apro il file di configurazione
	FILE *rescuers_conf = fopen(RESCUERS_CONF, "r");
	if(!rescuers_conf) { 
		perror("Errore apertura file di configurazione " RESCUERS_CONF);
		exit(EXIT_FAILURE);
	}

	// inizializzo l'array di rescuer_types dinamicamente a NULL
	rescuer_type_t **  rescuer_types = init_rescuer_types();

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
		if(line_count > MAX_FILE_LINES){
			perror("Numero massimo di linee superato file di configurazione " RESCUERS_CONF);
			exit(EXIT_FAILURE);
		}

		// Controllo che il numero massimo di rescuer non venga superato
		if(local_rescuer_count > MAX_RESCUER_COUNT){
			perror("Numero massimo di rescuer superato file di configurazione " RESCUERS_CONF);
			exit(EXIT_FAILURE);
		}

		IGNORE_EMPITY_LINES();

		// Controllo la lunghezza della riga
		if(strlen(line) > MAX_LINE_LENGTH){
			perror("Riga troppo lunga file di configurazione " RESCUERS_CONF);
			exit(EXIT_FAILURE);
		}

		// Controllo che la sintassi sia corretta ed estraggo i valori
		// conosco già il numero di campi e il loro tipo
		// quindi una sscanf basta per estrarre i valori
		if(sscanf(line, RESCUERS_SYNTAX , name, &amount, &speed, &x, &y) != 5){
			perror("Errore di sintassi file di configurazione " RESCUERS_CONF);
			exit(EXIT_FAILURE);
		}

		// Controllo che i valori siano validi
		if(rescuer_values_are_illegal(name, amount, speed, x, y)){
			perror("Valori illegali nel file di configurazione " RESCUERS_CONF);
			exit(EXIT_FAILURE);
		}

		// Controllo che non ci siano duplicati
		if(get_rescuer_type_by_name(name, rescuer_types)){ //il nome è già presente, errore
			perror("Presenza di duplicati in" RESCUERS_CONF);
			exit(EXIT_FAILURE);
		}
		
		// i campi sono validi e il nome non è presente, alloco il rescuer nel primo posto libero
		allocate_rescuer_type(name, amount, speed, x, y, rescuer_types); 

		local_rescuer_count++;
	}

	free(line);
	fclose(rescuers_conf);
	*rescuer_count = local_rescuer_count;

	return rescuer_types;
}

rescuer_type_t * get_rescuer_type_by_name(char *name, rescuer_type_t **rescuer_types){
	for(int i = 0; rescuer_types[i] != NULL; i++){
		if(strcmp(rescuer_types[i]->rescuer_type_name, name) == 0){
			return rescuer_types[i];
		}
	}
	return NULL;
}

int get_rescuer_type_index_by_name(char *name, rescuer_type_t **rescuer_types){
	for(int i = 0; rescuer_types[i] != NULL; i++){
		if(strcmp(rescuer_types[i]->rescuer_type_name, name) == 0){
			return i;
		}
	}
	return -1;
}

void allocate_rescuer_type(char *name, int amount, int speed, int x, int y, rescuer_type_t **rescuer_types){
	
	//raggiujgo il primo posto libero in rescuer_types
	int i = 0;
	while(rescuer_types[i] != NULL) i++;

	// allco il rescuer_type_t
	rescuer_types[i] = (rescuer_type_t *)malloc(sizeof(rescuer_type_t));
	if(!rescuer_types[i]) {
		perror("Errore allocazione memoria rescuer_types");
		exit(EXIT_FAILURE);
	}

	// alloco il nome del rescuer_type_t e lo copio
	rescuer_types[i]->rescuer_type_name = (char *)malloc((strlen(name) + 1) * sizeof(char));
	if(!rescuer_types[i]->rescuer_type_name) {
		perror("Errore allocazione memoria rescuer_type_name ");
		exit(EXIT_FAILURE);
	}
	strcpy(rescuer_types[i]->rescuer_type_name, name);

	// popolo il resto dei campi
	rescuer_types[i]->amount = amount;
	rescuer_types[i]->speed = speed;
	rescuer_types[i]->x = x;
	rescuer_types[i]->y = y;

	// alloco i rescuer_digital_twin_t (all'inizio tutti a NULL)
	rescuer_types[i]->twins = (rescuer_digital_twin_t **)calloc(amount + 1, sizeof(rescuer_digital_twin_t*));
	if(!rescuer_types[i]->twins) {
		perror("Errore allocazione memoria rescuer_digital_twins ");
		exit(EXIT_FAILURE);
	}

	// popolo i campi dei rescuer_digital_twin_t
	for(int j = 0; j < amount; j++){
		rescuer_types[i]->twins[j] = (rescuer_digital_twin_t *)malloc(sizeof(rescuer_digital_twin_t));
		if(!rescuer_types[i]->twins[j]) {
			perror("Errore allocazione memoria rescuer_digital_twin");
			exit(EXIT_FAILURE);
		}
		rescuer_types[i]->twins[j]->id = j;
		rescuer_types[i]->twins[j]->x = x;
		rescuer_types[i]->twins[j]->y = y;
		rescuer_types[i]->twins[j]->rescuer = rescuer_types[i];
		rescuer_types[i]->twins[j]->status = IDLE;
	}
}

rescuer_type_t ** init_rescuer_types(){
	rescuer_type_t **rescuer_types = (rescuer_type_t **)calloc((MAX_FILE_LINES + 1),  sizeof(rescuer_type_t*));
	if(!rescuer_types) {
		perror("Errore allocazione memoria rescuer_types");
		exit(EXIT_FAILURE);
	}
	return rescuer_types;
}

int rescuer_values_are_illegal(char *name, int amount, int speed, int x, int y){
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
