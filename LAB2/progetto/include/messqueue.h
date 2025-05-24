#ifndef MESSQUEUE_H
#define MESSQUEUE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>    
#include <sys/stat.h> 
#include <mqueue.h>
#include <unistd.h>

#include "structs.h"


#define MQ_SERVER_CLIENT "/mq_sc"
#define MAX_MESSAGES 1024
#define SC_QUEUE_MESSAGE_LENGTH 1024

// struttrura per inviare messaggi dal server al client
// all'inizio invia il nome della queue "emergenze" + numero matricola parsata da env.conf dando il via al client
// alla fine invia il segnale di chiudere tutto e non accettare più emergenze perchè è finito il programma

typedef struct {
	char server_string[SC_QUEUE_MESSAGE_LENGTH];
	int server_number;
} message_from_server_t;

typedef struct {
	emergency_request_t *emergency;
	log_event_info_t *event;
} message_from_client_t;




#endif