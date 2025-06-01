#ifndef UTILS_H
#define UTILS_H

#include <string.h>
#include <math.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "structs.h"

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

#define TRUNCATE_STRING_AT_MAX_LENGTH(s,maxlen) \
	do { \
		if(strlen(s) >= (maxlen)) \
			(s)[(maxlen)-1] = '\0'; \
	} while(0)

//error checking macros (le uniche macro del progetto che non sono in all caps)

#define check_error_minus_one(s,m) 	if ((s) == -1) 		{ perror(m); exit(EXIT_FAILURE); }
#define check_error_nonzero(s,m) 		if ((s) != 0)			{ perror(m); exit(EXIT_FAILURE); }
#define check_error_NULL(s,m) 			if ((s) == NULL) 	{ perror(m); exit(EXIT_FAILURE); }
#define check_error_not(s,m) 				if (!(s)) 				{ perror(m); exit(EXIT_FAILURE); }
#define check_error_condition(c, m) if ((c)) 					{ perror(m); exit(EXIT_FAILURE); }

#define check_error_mq_open(mq) 	if ((mq) == (mqd_t)-1) 	{ perror("mq_open"); 		exit(EXIT_FAILURE); }
#define check_error_mq_send(b) 		if ((b) == -1) 					{ perror("mq_send"); 		exit(EXIT_FAILURE); }
#define check_error_mq_recieve(b) if ((b) == -1) 					{ perror("mq_receive"); exit(EXIT_FAILURE); }

int my_atoi(char a[]);
void write_line(FILE *f, char *s);


#define ABS(x) ((x) < 0 ? -(x) : (x))
#define MANHATTAN(x1,y1,x2,y2) (ABS((x1) - (x2)) + ABS((y1) - (y2)))


#endif