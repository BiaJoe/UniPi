#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <ctype.h>
#include "../include/parsers.h"

#define RESCUERS_CONF "../conf/rescuers.conf"
#define MAX_LINE_LENGTH 1024
#define MAX_WORD_LENGTH 64
#define TOKEN_ENDING_CHARS "];:,="

rescuer_digital_twin_t ** parse_rescuers(){

	// Apro il file di configurazione
	FILE *rescuers_conf = fopen(RESCUERS_CONF, "r");
	if(!rescuers_conf) { 
		// TODO: gestire errore 
	}
	
	// Itero per ogni riga del file
	char *line = NULL;
	size_t len = 0;
	while (getline(&line, &len, rescuers_conf) != -1) {
		char** line_tokens = tokenize(&line);
	}

	free(line);
	fclose(rescuers_conf);

	
}

// Tokenizer: "[x][y][z]" -> {"x", "y", "z"}
char** tokenize(char *line){
	const char *tec = TOKEN_ENDING_CHARS; 
	int status = 0; 
	int i = 0;
	int j = 0;
	char *token = NULL;
	while(line[i] != '\n'){
		char buffer[MAX_WORD_LENGTH];
		char c = line[i];
		if(c == ' '){
			i++;
			continue;
		}
		if(strchr(tec, c)){
		 
		}


		switch(c){
			case ' ': status = 0; // Ignora gli spazi
				break;
			case '[': status = 1; // Inizio di un token
				break;
			case ']': status = 2; // Fine di un token
				break;
			case ';': status = 2;
				break;
			case ',': status = 2;
				break;
			case ':': status = 2;
				break;
			default:
				if(status == 2){
					// il token è finito, termino la stringa
					buffer[i] = '\0';
					 
				}
				else if(status == 1){
					// Copia il carattere nel buffer
					buffer[i] = c;
				}
		}
		i++;
	}
}