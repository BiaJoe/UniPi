#include "log.h"

// questo file ha tutte le funzioni necessarie per inviare il messaggio da loggare a logger.c


#define TERMINATING 		1
#define NOT_TERMINATING 0
#define LOG 						1
#define DONT_LOG 				0


// Lookup table per i possibili eventi di log,
// necessaria perchè per ogni evento serve il suo nome
// ed un codice univoco da associare ad un id numerico per non rischiare 
// di avere ID duplicati nel file di log.

static log_event_info_t log_event_lookup_table[LOG_EVENT_TYPES_COUNT] = {
	//	TIPO																				STRINGA																					CODICE (per l'ID)		CONTEGGIO		FA TERMINARE IL PROGRAMMA?		DA LOGGARE?
			[NON_APPLICABLE]                  				= { "NON_APPLICABLE",                  						"N/A ", 						0, 					NOT_TERMINATING, 							LOG 			},
			[FATAL_ERROR]                     				= { "FATAL_ERROR",                     						"ferr", 						0, 					TERMINATING, 		 							LOG 			},
			[FATAL_ERROR_PARSING]             				= { "FATAL_ERROR_PARSING",             						"fepa", 						0, 					TERMINATING, 		 							LOG 			},
			[FATAL_ERROR_LOGGING]             				= { "FATAL_ERROR_LOGGING",             						"felo", 						0, 					TERMINATING, 		 							LOG 			},
			[FATAL_ERROR_MEMORY]              				= { "FATAL_ERROR_MEMORY",              						"feme", 						0, 					TERMINATING, 		 							LOG 			},
			[FATAL_ERROR_FILE_OPENING]        				= { "FATAL_ERROR_FILE_OPENING",        						"fefo", 						0, 					TERMINATING, 		 							LOG 			},
			[EMPTY_CONF_LINE_IGNORED]         				= { "EMPTY_CONF_LINE_IGNORED",         						"ecli", 						0, 					NOT_TERMINATING, 							DONT_LOG 	},
			[DUPLICATE_RESCUER_REQUEST_IGNORED] 			= { "DUPLICATE_RESCUER_REQUEST_IGNORED", 					"drri", 						0, 					NOT_TERMINATING, 							LOG 			},
			[DUPLICATE_EMERGENCY_TYPE_IGNORED] 				= { "DUPLICATE_EMERGENCY_TYPE_IGNORED",						"deti", 						0, 					NOT_TERMINATING, 							LOG 			},
			[DUPLICATE_RESCUER_TYPE_IGNORED]  				= { "DUPLICATE_RESCUER_TYPE_IGNORED",  						"drti", 						0, 					NOT_TERMINATING, 							LOG 			},
			[WRONG_EMERGENCY_REQUEST_IGNORED_CLIENT] 	= { "WRONG_EMERGENCY_REQUEST_IGNORED_CLIENT", 		"werc", 						0, 					NOT_TERMINATING, 							LOG 			},
			[WRONG_EMERGENCY_REQUEST_IGNORED_SERVER] 	= { "WRONG_EMERGENCY_REQUEST_IGNORED_SERVER", 		"wers", 						0, 					NOT_TERMINATING, 							LOG 			},			
			[LOGGING_STARTED]                 				= { "LOGGING_STARTED",                 						"lsta", 						0, 					NOT_TERMINATING, 							LOG 			},
			[LOGGING_ENDED]														= { "LOGGING_ENDED",                   						"lend", 						0, 					NOT_TERMINATING, 							LOG 			},
			[PARSING_STARTED]                 				= { "PARSING_STARTED",                 						"psta", 						0, 					NOT_TERMINATING, 							LOG 			},
			[PARSING_ENDED]                   				= { "PARSING_ENDED",                   						"pend", 						0, 					NOT_TERMINATING, 							LOG 			},
			[RESCUER_TYPE_PARSED]             				= { "RESCUER_TYPE_PARSED",             						"rtpa", 						0, 					NOT_TERMINATING, 							LOG 			},
			[RESCUER_DIGITAL_TWIN_ADDED]      				= { "RESCUER_DIGITAL_TWIN_ADDED",      						"rdta", 						0, 					NOT_TERMINATING, 							LOG 			},
			[EMERGENCY_PARSED]                				= { "EMERGENCY_PARSED",                						"empa", 						0, 					NOT_TERMINATING, 							LOG 			},
			[RESCUER_REQUEST_ADDED]           				= { "RESCUER_REQUEST_ADDED",           						"rrad", 						0, 					NOT_TERMINATING, 							LOG 			},
			[SERVER]           												= { "SERVER",           													"srvr", 						0, 					NOT_TERMINATING, 							LOG 			},
			[CLIENT]           												= { "CLIENT",           													"clnt", 						0, 					NOT_TERMINATING, 							LOG 			},
			[EMERGENCY_REQUEST_RECEIVED]      				= { "EMERGENCY_REQUEST_RECEIVED",      						"errr", 						0, 					NOT_TERMINATING, 							LOG 			},
			[EMERGENCY_REQUEST_PROCESSED]     				= { "EMERGENCY_REQUEST_PROCESSED",     						"erpr", 						0, 					NOT_TERMINATING, 							LOG 			},
			[MESSAGE_QUEUE_CLIENT]                   	= { "MESSAGE_QUEUE_CLIENT",                   		"mqcl", 						0, 					NOT_TERMINATING, 							LOG 			},
			[MESSAGE_QUEUE_SERVER]                   	= { "MESSAGE_QUEUE_SERVER",                   		"mqse", 						0, 					NOT_TERMINATING, 							LOG 			},
			[EMERGENCY_STATUS]                				= { "EMERGENCY_STATUS",                						"esta", 						0, 					NOT_TERMINATING, 							LOG 			},
			[RESCUER_STATUS]                  				= { "RESCUER_STATUS",                  						"rsta", 						0, 					NOT_TERMINATING, 							LOG 			},
			[EMERGENCY_REQUEST]               				= { "EMERGENCY_REQUEST",               						"erre", 						0, 					NOT_TERMINATING, 							LOG 			},
			[PROGRAM_ENDED_SUCCESSFULLY]							= { "PROGRAM_ENDED_SUCCESSFULLY",									"pesu", 						0,					TERMINATING,  								LOG				}
	};

void send_log_message(char message[]) {
	// per non dover aprire e chiudere la coda di log ad OGNI chiamata
	// la dichiarop statica nella funzione, cioè verrà ricordata anche tra chiamate
	// dovrò aprirla quindi una sola volta (la prima quando if(mq == -1){...} è vero)
	static mqd_t mq = (mqd_t)-1;

	// entra solo la prima volta
	if(mq == (mqd_t)-1)
		check_error_mq_open(mq = mq_open(LOG_QUEUE_NAME, O_WRONLY));

	// mi assicuro che il messaggio non sia troppo lungo prima di inviarlo
	TRUNCATE_STRING_AT_MAX_LENGTH(message, MAX_LOG_EVENT_LENGTH);

	// invio il messaggio
	check_error_mq_send(mq_send(mq, message, strlen(message) + 1, 0));

	if(I_HAVE_TO_CLOSE_THE_LOG(message)){
		mq_close(mq); // non faccio unlink perchè lo fa il ricevitore	
		mq = (mqd_t)-1;
	}
}

// funzione chiamata da client; server per loggare un evento
void log_event(int id, log_event_type_t type, char *message) {

	// se l'evento non è da loggare non si logga 
	if(!is_log_event_type_to_log(type)) 
		return;

	// stringa da inviare
	char buffer[MAX_LOG_EVENT_LENGTH];
	
	// popolo la stringa da inviare con i dati del log
	snprintf(
		buffer, 
		MAX_LOG_EVENT_LENGTH, 
		LOG_EVENT_STRING_SYNTAX,
		get_time(),
		get_log_event_type_code(type),
		(id == NO_ID) ? get_log_event_type_counter(type) : id, 
		get_log_event_type_string(type), 
		message
	);

	send_log_message(buffer);
	increment_log_event_type_counter(type);

	// se l'evento è fatale si invia anche il messaggio di stop logging
	if(is_log_event_type_terminating(type)) 
		send_log_message(STOP_LOGGING_MESSAGE);
}



// funzioni di utilità per il logging

log_event_info_t* get_log_event_info(log_event_type_t event_type) {
	if (event_type >= 0 && event_type < LOG_EVENT_TYPES_COUNT) {
		return &log_event_lookup_table[event_type];
	}
	return &log_event_lookup_table[0];  // N/A
}

char* get_log_event_type_string(log_event_type_t event_type) {
	log_event_info_t *info = get_log_event_info(event_type);
	char *name = info->name;
	return name;
}

char* get_log_event_type_code(log_event_type_t event_type) {
	log_event_info_t* info = get_log_event_info(event_type);
	char *code = info->code;
	return code;
}

void increment_log_event_type_counter(log_event_type_t event_type){
	log_event_info_t *info = get_log_event_info(event_type);
	atomic_fetch_add(&(info->counter), 1);
}

int get_log_event_type_counter(log_event_type_t event_type) {
	log_event_info_t *info = get_log_event_info(event_type);
	int count = atomic_load(&(info->counter));
	return count;
}

int is_log_event_type_terminating(log_event_type_t event_type) {
	log_event_info_t *info = get_log_event_info(event_type);
	return info->is_terminating;
}

int is_log_event_type_to_log(log_event_type_t event_type) {
	log_event_info_t *info = get_log_event_info(event_type);
	return info->is_to_log;
}

long get_time() {
	return (long) time(NULL);
} 

// funzione per loggare un errore fatale e far terminare il programma
void log_fatal_error(char *message, log_event_type_t event) {
	// NO_ID perchè un errore fatale può accadere una sola volta
	// non si esce nè si fa log_close perchè ci pensa la funzione log_event
	// la funzione non fa molto ma è qui per essere espansa eventualmente in futuro
	// e per permettere a chi leggere il codice di capire cosa sta succedendo
	// non genera errori in caso di errore non fatale loggato perchè è solo un wrap di log_event
	log_event(NO_ID, event, message); 

	// solo se l'evento è sicuramente fatale per l'applicazione 
	// allora si termina il processo che ha loggato l'evento
	if(is_log_event_type_terminating(event)){
		perror(message);
		exit(EXIT_FAILURE);
	}
}

