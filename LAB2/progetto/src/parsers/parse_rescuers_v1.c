#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <ctype.h>
#include "../include/parsers.h"


rescuer_type_t** parse_rescuers(int *rescuer_count){

	// Apro il file di configurazione
	FILE *rescuers_conf = fopen(RESCUERS_CONF, "r");
	if(!rescuers_conf) { 
		perror("Errore apertura file di configurazione " RESCUERS_CONF);
		exit(EXIT_FAILURE);
	}


	char *line = NULL;
	size_t len = 0;
	char name[MAX_RESCUER_NAME_LENGTH]; // buffer per il singolo nome del rescuer
	int amount, speed, x, y;		// variabili temporanee per i campi del rescuer

	//buffer per i nomi dei rescuer inizializzato a'\0'
	char rescuer_names_buffer[MAX_FILE_LINES][MAX_RESCUER_NAME_LENGTH]; 
	for (int i = 0; i < MAX_FILE_LINES; i++) rescuer_names_buffer[i][0] = '\0'; 
	
	// buffer per i tipi di rescuer inizializzato a 0
	rescuer_type_t rescuer_types_buffer[MAX_FILE_LINES];
	for(int i = 0; i<MAX_FILE_LINES; i++){
		rescuer_types_buffer[i].rescuer_type_name = NULL;
		rescuer_types_buffer[i].amount = 0;
		rescuer_types_buffer[i].speed = 0;
		rescuer_types_buffer[i].x = 0;
		rescuer_types_buffer[i].y = 0;
		rescuer_types_buffer[i].twins = NULL;
	}
	
	// contatori di linee e numero di rescuer
	int local_rescuer_count = 0;
	int line_count = 0;

	// Leggo ogni riga del file e processo le informazioni contenute
	while (getline(&line, &len, rescuers_conf) != -1) {
		line_count++;

		// Controllo che il numero massimo di linee non venga superato
		if(line_count > MAX_FILE_LINES){
			perror("Numero massimo di linee superato file di configurazione " RESCUERS_CONF);
			exit(EXIT_FAILURE);
		}

		// Ignoro le righe vuote
		if (line[0] == '\n') continue;

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
		if(
			strlen(name) <= 0 || 
			amount <= 0 || 
			speed <= 0 || 
			x < 0 || 
			y < 0
		){
			perror("Valori illegali file di configurazione " RESCUERS_CONF);
			exit(EXIT_FAILURE);
		}

		// Arrivati qui il rescuer è valido
		// Controllo che non ci siano duplicati
		// Se non ci sono il nome viene aggiunto al buffer dei nomi
		if(rescuer_arleady_exists(name, rescuer_names_buffer)){
			perror("Presenza di duplicati in" RESCUERS_CONF);
			exit(EXIT_FAILURE);
		}

		// Aggiungo il rescuer al buffer dei rescuer
		// Il buffer servirà dopo per l'allocazione dinamica
		// Il nome non viene copiato in questa fase perchè è già presente nel buffer dei nomi
		// Non aggiungo il nome
		rescuer_types_buffer[local_rescuer_count].amount = amount;
		rescuer_types_buffer[local_rescuer_count].speed = speed;
		rescuer_types_buffer[local_rescuer_count].x = x;
		rescuer_types_buffer[local_rescuer_count].y = y;

		local_rescuer_count++;

		// non ci sono più di MAX_FILE_LINES rescuer e le righe ignorate non creano buchi nel buffer dei rescuer
	}

	free(line);
	fclose(rescuers_conf);

	// adesso ho i rescuer in un buffer e i nomi in un altro
	// devo:
	// - allocare dinamicamente l'array di rescuer_types
	// - copiare i nomi dei rescuer da rescuer_names_buffer nell'array di rescuer_types
	// - generare gli array di rescuer_digital_twins

	// Alloco dinamicamente l'array di rescuer_types
	rescuer_type_t ** rescuer_types = (rescuer_type_t **)malloc((local_rescuer_count + 1) * sizeof(rescuer_type_t*));
	if(!rescuer_types) {
		perror("Errore allocazione memoria rescuer_types");
		exit(EXIT_FAILURE);
	}

	// Popolo con i nomi, i campi e i puntatori a rescuer_digital_twins
	for(int i = 0; i < local_rescuer_count; i++){
		rescuer_types[i] = (rescuer_type_t *)malloc(sizeof(rescuer_type_t));
		if(!rescuer_types[i]) {
			perror("Errore allocazione memoria rescuer_types");
			exit(EXIT_FAILURE);
		}

		// Alloco la memoria necessaria e copio il nome del rescuer
		rescuer_types[i]->rescuer_type_name = (char *)malloc((strlen(rescuer_names_buffer[i]) + 1) * sizeof(char));
		strcpy(rescuer_types[i]->rescuer_type_name, rescuer_names_buffer[i]);

		// Uso variabili temporanee per maggiore leggibilità
		amount 		= rescuer_types_buffer[i].amount;
		speed 		= rescuer_types_buffer[i].speed;
		x 				= rescuer_types_buffer[i].x;
		y 				= rescuer_types_buffer[i].y;

		// Copio i campi interi
		rescuer_types[i]->amount 	= amount;
		rescuer_types[i]->speed 	= speed;
		rescuer_types[i]->x 			= x;
		rescuer_types[i]->y 			= y;

		// Alloco la memoria per i rescuer_digital_twins
		rescuer_types[i]->twins = (rescuer_digital_twin_t **)malloc((amount + 1) * sizeof(rescuer_digital_twin_t*));
		if(!rescuer_types[i]->twins) {
			perror("Errore allocazione memoria rescuer_digital_twins");
			exit(EXIT_FAILURE);
		}

		// Alloco la memoria per ogni rescuer_digital_twin e lo popolo
		for(int j = 0; j < amount; j++){
			rescuer_types[i]->twins[j] = (rescuer_digital_twin_t *)malloc(sizeof(rescuer_digital_twin_t));
			if(rescuer_types[i]->twins[j] == NULL) {
				perror("Errore allocazione memoria rescuer_digital_twins");
				exit(EXIT_FAILURE);
			}
			rescuer_types[i]->twins[j]->id = j;
			rescuer_types[i]->twins[j]->x = x;
			rescuer_types[i]->twins[j]->y = y;
			rescuer_types[i]->twins[j]->rescuer = rescuer_types[i];
			rescuer_types[i]->twins[j]->status = IDLE;
		}
		// Aggiungo il terminatore dell'array
		rescuer_types[i]->twins[amount] = NULL;
	}

	// Aggiungo il terminatore dell'array
	rescuer_types[local_rescuer_count] = NULL;
	// Restituisco l'array di rescuer_types
	*rescuer_count = local_rescuer_count;
	return rescuer_types;
}








// Funzione che controlla se il nome del rescuer è già presente nel buffer
int rescuer_arleady_exists(char *name, char rescuer_names_buffer[][MAX_RESCUER_NAME_LENGTH]){
	int last_index = 0;

	for(int i = 0; i < MAX_FILE_LINES; i++){

		// se siamo al primo posto libero (abbiamo finito i nomi da controllare)
		if(rescuer_names_buffer[i][0] == '\0'){ 
			last_index = i;	
			break;
		}

		// Se il nome è già presente
		if(strcmp(name, rescuer_names_buffer[i]) == 0){ 
			return 1;
		}
	}

	// Se non è presente lo aggiungo alla fine del buffer
	strcpy(rescuer_names_buffer[last_index], name);
	return 0;
}
