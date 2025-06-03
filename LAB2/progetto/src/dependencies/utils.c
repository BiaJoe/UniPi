#include "utils.h"

int my_atoi(char a[]){
	int order = strlen(a) - 1;
	int res = 0;
	char c;
	int i = 0;
	int is_negative = 0;

	// numero negativo o positivo?
	if((a[0]) == '-'){ 
		i = 1;
		is_negative = 1;
		order--;
	}

	// controllo che la prima cifra (i = 0 o i = 1) non sia zero
	if((a[i]) == '0'){ 
		errno = EINVAL;
		return 0;
	}

	// scorro il numero
	while((c = a[i]) != '\0'){
		// controllo che la cifra sia numerica
		if(c < '0' || c > '9'){
			errno = EINVAL;
			return 0;
		}
		// aggiorno il risultato
		res += (c - '0') * ((int)pow(10, order--));
	}

	if(is_negative) 
		return -res;
	
	return res;
}


void write_line(FILE *f, char *s) {
	check_error_fopen(f);
	fprintf(f, "%s", s);
	fflush(f);
}