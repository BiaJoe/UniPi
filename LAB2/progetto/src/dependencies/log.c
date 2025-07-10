#include "log.h"

// questo file ha tutte le funzioni necessarie per inviare il messaggio da loggare a logger.c

// Lookup table per i possibili eventi di log,
// necessaria perchè per ogni evento serve il suo nome
// ed un codice univoco da associare ad un id numerico per non rischiare 
// di avere ID duplicati nel file di log.
// il codice non lo userò perchè non fa parte delle specifiche richieste, ma è un'opzione

static log_event_info_t log_event_lookup_table[LOG_EVENT_TYPES_COUNT] = {
	//	TIPO																				STRINGA																					CODICE (per l'ID)		CONTEGGIO		FA TERMINARE IL PROGRAMMA?		DA LOGGARE?
			[NON_APPLICABLE]                  				= { "NON_APPLICABLE",                  						"N/A ", 						0, 					NO, 													YES 			},
			[FATAL_ERROR]                     				= { "FATAL_ERROR",                     						"ferr", 						0, 					YES, 		 											YES 			},
			[FATAL_ERROR_PARSING]             				= { "FATAL_ERROR_PARSING",             						"fepa", 						0, 					YES, 		 											YES 			},
			[FATAL_ERROR_LOGGING]             				= { "FATAL_ERROR_LOGGING",             						"felo", 						0, 					YES, 		 											YES 			},
			[FATAL_ERROR_MEMORY]              				= { "FATAL_ERROR_MEMORY",              						"feme", 						0, 					YES, 		 											YES 			},
			[FATAL_ERROR_FILE_OPENING]        				= { "FATAL_ERROR_FILE_OPENING",        						"fefo", 						0, 					YES, 		 											YES 			},
			[EMPTY_CONF_LINE_IGNORED]         				= { "EMPTY_CONF_LINE_IGNORED",         						"ecli", 						0, 					NO, 													NO 				},	
			[DUPLICATE_RESCUER_REQUEST_IGNORED] 			= { "DUPLICATE_RESCUER_REQUEST_IGNORED", 					"drri", 						0, 					NO, 													YES 			},
			[WRONG_RESCUER_REQUEST_IGNORED] 					= { "WRONG_RESCUER_REQUEST_IGNORED", 							"wrri", 						0, 					NO, 													YES 			},
			[DUPLICATE_EMERGENCY_TYPE_IGNORED] 				= { "DUPLICATE_EMERGENCY_TYPE_IGNORED",						"deti", 						0, 					NO, 													YES 			},
			[DUPLICATE_RESCUER_TYPE_IGNORED]  				= { "DUPLICATE_RESCUER_TYPE_IGNORED",  						"drti", 						0, 					NO, 													YES 			},
			[WRONG_EMERGENCY_REQUEST_IGNORED_CLIENT] 	= { "WRONG_EMERGENCY_REQUEST_IGNORED_CLIENT", 		"werc", 						0, 					NO, 													YES 			},
			[WRONG_EMERGENCY_REQUEST_IGNORED_SERVER] 	= { "WRONG_EMERGENCY_REQUEST_IGNORED_SERVER", 		"wers", 						0, 					NO, 													YES 			},			
			[LOGGING_STARTED]                 				= { "LOGGING_STARTED",                 						"lsta", 						0, 					NO, 													YES 			},
			[LOGGING_ENDED]														= { "LOGGING_ENDED",                   						"lend", 						0, 					NO, 													YES 			},
			[PARSING_STARTED]                 				= { "PARSING_STARTED",                 						"psta", 						0, 					NO, 													YES 			},
			[PARSING_ENDED]                   				= { "PARSING_ENDED",                   						"pend", 						0, 					NO, 													YES 			},
			[RESCUER_TYPE_PARSED]             				= { "RESCUER_TYPE_PARSED",             						"rtpa", 						0, 					NO, 													YES 			},
			[RESCUER_DIGITAL_TWIN_ADDED]      				= { "RESCUER_DIGITAL_TWIN_ADDED",      						"rdta", 						0, 					NO, 													YES 			},
			[EMERGENCY_PARSED]                				= { "EMERGENCY_PARSED",                						"empa", 						0, 					NO, 													YES 			},
			[RESCUER_REQUEST_ADDED]           				= { "RESCUER_REQUEST_ADDED",           						"rrad", 						0, 					NO, 													YES 			},
			[SERVER]           												= { "SERVER",           													"srvr", 						0, 					NO, 													YES 			},
			[CLIENT]           												= { "CLIENT",           													"clnt", 						0, 					NO, 													YES 			},
			[EMERGENCY_REQUEST_RECEIVED]      				= { "EMERGENCY_REQUEST_RECEIVED",      						"errr", 						0, 					NO, 													YES 			},
			[EMERGENCY_REQUEST_PROCESSED]     				= { "EMERGENCY_REQUEST_PROCESSED",     						"erpr", 						0, 					NO, 													YES 			},
			[MESSAGE_QUEUE_CLIENT]                   	= { "MESSAGE_QUEUE_CLIENT",                   		"mqcl", 						0, 					NO, 													YES 			},
			[MESSAGE_QUEUE_SERVER]                   	= { "MESSAGE_QUEUE_SERVER",                   		"mqse", 						0, 					NO, 													YES 			},
			[EMERGENCY_STATUS]                				= { "EMERGENCY_STATUS",                						"esta", 						0, 					NO, 													YES 			},
			[RESCUER_STATUS]                  				= { "RESCUER_STATUS",                  						"rsta", 						0, 					NO, 													YES 			},
			[RESCUER_TRAVELLING_STATUS]               = { "RESCUER_TRAVELLING_STATUS",                  "rtst", 						0, 					NO, 													NO 				},
			[EMERGENCY_REQUEST]               				= { "EMERGENCY_REQUEST",               						"erre", 						0, 					NO, 													YES 			},
			[PROGRAM_ENDED_SUCCESSFULLY]							= { "PROGRAM_ENDED_SUCCESSFULLY",									"pesu", 						0,					YES,  												YES				}
};


// funzione che invia il messaggio da loggare al processo logger.c 
// attraverso la message queue
// se si vuol far terminare il logging è sufficiente loggare un evento "terminatore"
// ad esempio PROGRAM_ENDED_SUCCESSFULLY fa terminare il programma
void send_log_message(char buffer[], long long time) {
	// per non dover aprire e chiudere la coda di log ad OGNI chiamata
	// la dichiarop statica nella funzione, cioè verrà ricordata anche tra chiamate
	// dovrò aprirla quindi una sola volta (la prima, quando if(mq == -1){...} è vero)
	static mqd_t mq = (mqd_t)-1;

	// entra solo la prima volta
	if(mq == (mqd_t)-1)
		try_to_open_queue(mq, LOG_QUEUE_NAME, HOW_MANY_ATTEMPTS_FOR_OPENING_LOG_QUEUE, NANOSECONDS_BETWEN_LOG_QUEUE_OPENING_ATTEMPTS)

	
	log_message_t msg;
	msg.timestamp = time;
	strncpy(msg.message, buffer, sizeof(msg.message)); // buffer è il messaggio formattato

	check_error_mq_send(mq_send(mq, (char*)&msg, sizeof(msg), 0));

	if(I_HAVE_TO_CLOSE_THE_LOG(buffer)){
		mq_close(mq); // non faccio unlink perchè lo fa il ricevitore	
		mq = (mqd_t)-1;
	}
}

// funzione chiamata dai processi client e server per loggare un evento
// Ho letto come avere un numero variabile di argomenti su :
// The C programming language di B. W. Kernighan e D. M. Ritchie
void log_event(int id, log_event_type_t type, char *format, ...) { 	
	if(!is_log_event_type_to_log(type)) 							// se l'evento non è da loggare non si logga 
		return;	
	
	char message[LOG_EVENT_MESSAGE_LENGTH];						// contiene il messaggio formattato con le variabili 
	va_list ap;																				// punta agli argomenti variabili unnamed
	va_start(ap, format);															// punta al primo argomento variabile
	vsnprintf(message, sizeof(message), format, ap);	// scrive il messaggio formattato nella stringa
	va_end(ap);																				// cleanup

	char id_string[MAX_LOG_EVENT_ID_STRING_LENGTH];		// se l'id è un caso speciale allora lo converto, altrimenti lo uso direttamente
	switch (id) {
		case AUTOMATIC_LOG_ID: 			snprintf(id_string, sizeof(id_string), "%d", get_log_event_type_counter(type)); break;
		case NON_APPLICABLE_LOG_ID:	snprintf(id_string, sizeof(id_string), "%s", NON_APPLICABLE_LOG_ID_STRING); break; 	
		default:										snprintf(id_string, sizeof(id_string), "%d", id);
	}
		
	char buffer[MAX_LOG_EVENT_LENGTH];
	long long time_nanoseconds;
	
	struct timespec ts;	// calcolo in che momento esatto stiamo loggando
	clock_gettime(CLOCK_REALTIME, &ts);
	time_nanoseconds = ts.tv_sec * 1000000000LL + ts.tv_nsec; // scrivo i nanosecondi

	snprintf(			
		buffer, 																// stringa che invierò a logger (message queue)
		MAX_LOG_EVENT_LENGTH / sizeof(char), 		// lunghezza massima fino a cui scrivere
		LOG_EVENT_STRING_SYNTAX,								// formato da dare alla stringa
		time(NULL),															// timestamp al momento esatto del log
		id_string, 															// id dell'evento già elaborato sopra
		get_log_event_type_string(type), 				// nome dell'evento
		message																	// messaggio da loggare				
	);

	printf("%s", buffer);

	send_log_message(buffer, time_nanoseconds);
	increment_log_event_type_counter(type);

	if(is_log_event_type_terminating(type)) // se l'evento fa terminare il programma si invia anche il messaggio di stop logging per far terminare il logger
		send_log_message(STOP_LOGGING_MESSAGE, time_nanoseconds);
}

void log_fatal_error(char *format, ...){
	char message[LOG_EVENT_MESSAGE_LENGTH];						// contiene il messaggio formattato con le variabili 
	va_list ap;																				// punta agli argomenti variabili unnamed
	va_start(ap, format);															// punta al primo argomento variabile
	vsnprintf(message, sizeof(message), format, ap);	// scrive il messaggio formattato nella stringa
	va_end(ap);
	log_event(NON_APPLICABLE_LOG_ID, FATAL_ERROR, message);
	perror(message);
	exit(EXIT_FAILURE);
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

