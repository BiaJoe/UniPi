#include "logger.h"

static FILE *log_file = NULL;

/*
Processo che riceve messaggi con message queue e li logga con log.c
*/

void logger(void){

	//inizializza il loggging
	log_init(); 

	//riceve i messaggi
	mqd_t mq;
	struct mq_attr attr;
	char buffer[MAX_LOG_EVENT_LENGTH];

	attr.mq_flags = 0;
	attr.mq_maxmsg = MAX_LOG_QUEUE_MESSAGES;
	attr.mq_msgsize = MAX_LOG_EVENT_LENGTH;
	attr.mq_curmsgs = 0;

	check_error_mq_open(mq = mq_open(LOG_QUEUE_NAME, O_CREAT | O_RDONLY, 0644, &attr));

	// ricevo i messaggi
	// se sono il messaggio di stop esco perché per qualche motivo ho finito
	// altrimenti li scrivo nel logfile
	while (1) {
		check_error_mq_recieve(mq_receive(mq, buffer, MAX_LOG_EVENT_LENGTH, NULL));
		if(I_HAVE_TO_CLOSE_THE_LOG(buffer)) break;
		write_line(log_file, buffer);
	}

	mq_close(mq);
	mq_unlink(LOG_QUEUE_NAME);

	log_close(); 

	exit(EXIT_SUCCESS);
}

void log_init() {
	check_error_fopen(log_file = fopen(LOG_FILE, "a"));
}

void log_close() {
	if (log_file) fclose(log_file);
}

