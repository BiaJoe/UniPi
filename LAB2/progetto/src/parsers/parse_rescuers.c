#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <ctype.h>
#include "../include/parsers.h"

#define RESCUERS_CONF "../conf/rescuers.conf"
#define MAX_LINE_LENGTH 1024
#define MAX_WORD_LENGTH 64
#define TOKEN_ENDING_CHARS "];:,="
#define TO_IGNORE_CHARS "[ \t\r"

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

// Tokenizer: "[x][y][z] a:b,c;d;" -> {"x", "y", "z", "a", "b", "c", "d"}
char** tokenize(char *line){
	char *token = NULL;
	char buffer[MAX_WORD_LENGTH];
	const char *tec = TOKEN_ENDING_CHARS; 
	const char *tic = TO_IGNORE_CHARS;
	int status = 0; 
	int i = 0;
	int j = 0;
	while(line[i] != '\n' && line[i] != '\0' && i < MAX_LINE_LENGTH && j < MAX_WORD_LENGTH){
		char c = line[i];
		if(strchr(tic, c)){
			i++;
			break;
		}
		if(strchr(tec, c)){
			status = 1;
			buffer[j] = '\0';
			i++;
			j++;
			break;
		}
		buffer[j] = c;
		j++;
		i++;


	}
}