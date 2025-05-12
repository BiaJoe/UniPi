


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_NAME_LENGTH 63
#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)
#define RESCUERS_SYNTAX "[%" STR(MAX_NAME_LENGTH) "][%d][%d][%d:%d]"


int main(){

	char buffer[64];

	buffer[0] = 'H';
	buffer[1] = 'e';
	buffer[2] = 'l';
	buffer[3] = 'l';		
	buffer[4] = 'o';
	buffer[5] = '\0';

	printf("%s\n", buffer);

	char * buffer2 = (char*)malloc((strlen(buffer)+1)*sizeof(char));
	strcpy(buffer2, buffer);

	printf("%s\n", buffer2);
	free(buffer2);
	printf(RESCUERS_SYNTAX "\n", 1, 2, 3, 4);

	return 0;
}