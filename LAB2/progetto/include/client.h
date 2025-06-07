#ifndef CLIENT_H
#define CLIENT_H

#define _GNU_SOURCE // per usare getline

#include <unistd.h>
#include "logger.h"
#include "memory_management.h"

#define LOG_IGNORING_ERROR(m) \
	do { log_event(NO_ID, WRONG_EMERGENCY_REQUEST_IGNORED_CLIENT, "il Client ignora una emergency request: " #m); } while (0)

#define PRINT_CLIENT_USAGE(argv0)  \
	do { \
		printf("Utilizzo: \n"); \
		printf("%s <nome_emergenza> <coord_x> <coord_y> <delay_in_secs>       (inserimento diretto) \n", argv0); \
		printf("%s -f <nome_file>                                             (leggi da file)       \n", argv0); \
	} while(0)

// scorciatoria solo per il main() di client.c
#define DIE(last_words) \
	do { \
		LOG_IGNORING_ERROR(last_words); \
		PRINT_CLIENT_USAGE(argv[0]); \
		exit(EXIT_FAILURE); \
	} while(0)

#define UNDEFINED_MODE -1
#define NORMAL_MODE 1
#define FILE_MODE 2

#define MAX_EMERGENCY_REQUEST_COUNT 256 
#define MAX_EMERGENCY_REQUEST_LENGTH 512

void handle_normal_mode_input(char* args[]);
void handle_file_mode_input(char* args[]);
int send_emergency_request_message(char *name, char *x_string, char *y_string, char *delay_string);

#endif

