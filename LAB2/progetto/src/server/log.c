#include "log.h"


#define FATAL 1
#define NOT_FATAL 0
#define LOG 1
#define DONT_LOG 0

static FILE *log_file = NULL;
static mtx_t log_mutex;

// Lookup table per i possibili eventi di log,
// necessaria perchè per ogni evento serve il suo nome
// ed un codice univoco da associare ad un id numerico per non rischiare 
// di avere ID duplicati nel file di log.
// static log_event_info_t log_event_lookup_table[LOG_EVENT_TYPES_COUNT] = {

//idea: aggiungere un counter per ogni tipo di evento.
//Parte da 0 e si incrementa ad ogni logging dell'evento
//andrebbe reso atomico oppure protetto con un mutex ma è possibile
//il controllo sarebbe centralizzato.

static log_event_info_t log_event_lookup_table[LOG_EVENT_TYPES_COUNT] = {
	[NON_APPLICABLE]                  	= { "NON_APPLICABLE",                  		"N/A"	, 0, NOT_FATAL, LOG },
	[FATAL_ERROR]                     	= { "FATAL_ERROR",                     		"ferr", 0, FATAL, 		LOG },
	[FATAL_ERROR_PARSING]             	= { "FATAL_ERROR_PARSING",             		"fepa", 0, FATAL, 		LOG },
	[FATAL_ERROR_LOGGING]             	= { "FATAL_ERROR_LOGGING",             		"felo", 0, FATAL, 		LOG },
	[FATAL_ERROR_MEMORY]              	= { "FATAL_ERROR_MEMORY",              		"feme", 0, FATAL, 		LOG },
	[FATAL_ERROR_FILE_OPENING]        	= { "FATAL_ERROR_FILE_OPENING",        		"fefo", 0, FATAL, 		LOG },
	[EMPTY_CONF_LINE_IGNORED]         	= { "EMPTY_CONF_LINE_IGNORED",         		"ecli", 0, NOT_FATAL, DONT_LOG },
	[DUPLICATE_RESCUER_REQUEST_IGNORED] = { "DUPLICATE_RESCUER_REQUEST_IGNORED", 	"drri", 0, NOT_FATAL, LOG },
	[DUPLICATE_EMERGENCY_TYPE_IGNORED] 	= { "DUPLICATE_EMERGENCY_TYPE_IGNORED",		"deti", 0, NOT_FATAL, LOG },
	[DUPLICATE_RESCUER_TYPE_IGNORED]  	= { "DUPLICATE_RESCUER_TYPE_IGNORED",  		"drti", 0, NOT_FATAL, LOG },
	[WRONG_EMERGENCY_REQUEST_IGNORED] 	= { "WRONG_EMERGENCY_REQUEST_IGNORED", 		"weri", 0, NOT_FATAL, LOG },
	[LOGGING_STARTED]                 	= { "LOGGING_STARTED",                 		"lsta", 0, NOT_FATAL, LOG },
	[LOGGING_ENDED]											= { "LOGGING_ENDED",                   		"lend", 0, NOT_FATAL, LOG },
	[PARSING_STARTED]                 	= { "PARSING_STARTED",                 		"psta", 0, NOT_FATAL, LOG },
	[PARSING_ENDED]                   	= { "PARSING_ENDED",                   		"pend", 0, NOT_FATAL, LOG },
	[RESCUER_TYPE_PARSED]             	= { "RESCUER_TYPE_PARSED",             		"rtpa", 0, NOT_FATAL, LOG },
	[RESCUER_DIGITAL_TWIN_ADDED]      	= { "RESCUER_DIGITAL_TWIN_ADDED",      		"rdta", 0, NOT_FATAL, LOG },
	[EMERGENCY_PARSED]                	= { "EMERGENCY_PARSED",                		"empa", 0, NOT_FATAL, LOG },
	[RESCUER_REQUEST_ADDED]           	= { "RESCUER_REQUEST_ADDED",           		"rrad", 0, NOT_FATAL, LOG },
	[EMERGENCY_REQUEST_RECEIVED]      	= { "EMERGENCY_REQUEST_RECEIVED",      		"errr", 0, NOT_FATAL, LOG },
	[EMERGENCY_REQUEST_PROCESSED]     	= { "EMERGENCY_REQUEST_PROCESSED",     		"erpr", 0, NOT_FATAL, LOG },
	[MESSAGE_QUEUE]                   	= { "MESSAGE_QUEUE",                   		"mque", 0, NOT_FATAL, LOG },
	[EMERGENCY_STATUS]                	= { "EMERGENCY_STATUS",                		"esta", 0, NOT_FATAL, LOG },
	[RESCUER_STATUS]                  	= { "RESCUER_STATUS",                  		"rsta", 0, NOT_FATAL, LOG },
	[EMERGENCY_REQUEST]               	= { "EMERGENCY_REQUEST",               		"erre", 0, NOT_FATAL, LOG }
	
};

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

long get_time() {
	return (long) time(NULL);
}

void log_init() {
	log_file = fopen(LOG_FILE, "w");
	if(!log_file) {
		perror("Errore apertura file di log");
		exit(EXIT_FAILURE);
	}

	if(mtx_init(&log_mutex, mtx_plain) != thrd_success) {
		perror("Errore inizializzazione mutex per il file di log");
		exit(EXIT_FAILURE);
	}

	log_event(0, LOGGING_STARTED, "Inizio logging");
}

void log_close() {
	log_event(0, LOGGING_ENDED, "Fine del logging");
	if (log_file) fclose(log_file);
}

void log_event(int id, log_event_type_t e, char *message) {

	// se l'evento non è da loggare non si logga 
	if(!log_event_lookup_table[e].to_log) return;

	mtx_lock(&log_mutex);

	if (!log_file) {
		perror("File di log non aperto");
		exit(EXIT_FAILURE);
	}

	log_event_lookup_table[e].counter++;

	int ID = (id == NO_ID) ? log_event_lookup_table[e].counter : id;

	long time = get_time();
	fprintf(
		log_file,
		LOG_EVENT_STRING, 
		time, get_log_event_type_code(e), 
		ID, 
		get_log_event_type_string(e), 
		message
	);

	fflush(log_file);

	mtx_unlock(&log_mutex);

	//se l'evento è fatale dopo averlo loggato si esce
	if(log_event_lookup_table[e].is_fatal){
		log_close();
		exit(EXIT_FAILURE);
	}
}
