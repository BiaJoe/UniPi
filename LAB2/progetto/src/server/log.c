#include "log.h"

static FILE *log_file = NULL;
static mtx_t log_mutex;

// Lookup table per i possibili eventi di log,
// necessaria perchè per ogni evento serve il suo nome
// ed un codice univoco da associare ad un id numerico per non rischiare 
// di avere ID duplicati nel file di log.
// static log_event_info_t log_event_lookup_table[LOG_EVENT_TYPES_COUNT] = {
// 	[NON_APPLICABLE]                  = { "NON_APPLICABLE",                  "NA"  },
// 	[FATAL_ERROR]                     = { "FATAL_ERROR",                     "FERR" },
// 	[FATAL_ERROR_PARSING]             = { "FATAL_ERROR_PARSING",             "FEPA" },
// 	[FATAL_ERROR_LOGGING]             = { "FATAL_ERROR_LOGGING",             "FELO" },
// 	[FATAL_ERROR_MEMORY]              = { "FATAL_ERROR_MEMORY",              "FEME" },
// 	[FATAL_ERROR_FILE_OPENING]        = { "FATAL_ERROR_FILE_OPENING",        "FEFO" },

// 	[EMPTY_CONF_LINE_IGNORED]         = { "EMPTY_CONF_LINE_IGNORED",         "ECLI" },
// 	[DUPLICATE_RESCUER_REQUEST_IGNORED] = { "DUPLICATE_RESCUER_REQUEST_IGNORED", "DRRI" },
// 	[DUPLICATE_EMERGENCY_TYPE_IGNORED] = { "DUPLICATE_EMERGENCY_TYPE_IGNORED", "DETI" },
// 	[DUPLICATE_RESCUER_TYPE_IGNORED]  = { "DUPLICATE_RESCUER_TYPE_IGNORED",  "DRTI" },
// 	[WRONG_EMERGENCY_REQUEST_IGNORED] = { "WRONG_EMERGENCY_REQUEST_IGNORED", "WERI" },

// 	[LOGGING_STARTED]                 = { "LOGGING_STARTED",                 "LSTA" },
// 	[LOGGING_ENDED]                   = { "LOGGING_ENDED",                   "LEND" },

// 	[PARSING_STARTED]                 = { "PARSING_STARTED",                 "PSTA" },
// 	[PARSING_ENDED]                   = { "PARSING_ENDED",                   "PEND" },
// 	[RESCUER_TYPE_PARSED]             = { "RESCUER_TYPE_PARSED",             "RTPA" },
// 	[RESCUER_DIGITAL_TWIN_ADDED]      = { "RESCUER_DIGITAL_TWIN_ADDED",      "RDTA" },
// 	[EMERGENCY_PARSED]                = { "EMERGENCY_PARSED",                "EMPA" },
// 	[RESCUER_REQUEST_ADDED]           = { "RESCUER_REQUEST_ADDED",           "RRAD" },

// 	[EMERGENCY_REQUEST_RECEIVED]      = { "EMERGENCY_REQUEST_RECEIVED",      "ERRR" },
// 	[EMERGENCY_REQUEST_PROCESSED]     = { "EMERGENCY_REQUEST_PROCESSED",     "ERPR" },

// 	[MESSAGE_QUEUE]                   = { "MESSAGE_QUEUE",                   "MQUE" },
// 	[EMERGENCY_STATUS]                = { "EMERGENCY_STATUS",                "ESTA" },
// 	[RESCUER_STATUS]                  = { "RESCUER_STATUS",                  "RSTA" },
// 	[EMERGENCY_REQUEST]               = { "EMERGENCY_REQUEST",               "ERRE" }
	
// };

//idea: aggiungere un counter per ogni tipo di evento.
//Parte da 0 e si incrementa ad ogni logging dell'evento
//andrebbe reso atomico oppure protetto con un mutex ma è possibile
//il controllo sarebbe centralizzato.

static log_event_info_t log_event_lookup_table[LOG_EVENT_TYPES_COUNT] = {
	[NON_APPLICABLE]                  	= { "NON_APPLICABLE",                  		"N/A"  },
	[FATAL_ERROR]                     	= { "FATAL_ERROR",                     		"ferr" },
	[FATAL_ERROR_PARSING]             	= { "FATAL_ERROR_PARSING",             		"fepa" },
	[FATAL_ERROR_LOGGING]             	= { "FATAL_ERROR_LOGGING",             		"felo" },
	[FATAL_ERROR_MEMORY]              	= { "FATAL_ERROR_MEMORY",              		"feme" },
	[FATAL_ERROR_FILE_OPENING]        	= { "FATAL_ERROR_FILE_OPENING",        		"fefo" },
	[EMPTY_CONF_LINE_IGNORED]         	= { "EMPTY_CONF_LINE_IGNORED",         		"ecli" },
	[DUPLICATE_RESCUER_REQUEST_IGNORED] = { "DUPLICATE_RESCUER_REQUEST_IGNORED", 	"drri" },
	[DUPLICATE_EMERGENCY_TYPE_IGNORED] 	= { "DUPLICATE_EMERGENCY_TYPE_IGNORED",		"deti" },
	[DUPLICATE_RESCUER_TYPE_IGNORED]  	= { "DUPLICATE_RESCUER_TYPE_IGNORED",  		"drti" },
	[WRONG_EMERGENCY_REQUEST_IGNORED] 	= { "WRONG_EMERGENCY_REQUEST_IGNORED", 		"weri" },
	[LOGGING_STARTED]                 	= { "LOGGING_STARTED",                 		"lsta" },
	[LOGGING_ENDED]                   	= { "LOGGING_ENDED",                   		"lend" },
	[PARSING_STARTED]                 	= { "PARSING_STARTED",                 		"psta" },
	[PARSING_ENDED]                   	= { "PARSING_ENDED",                   		"pend" },
	[RESCUER_TYPE_PARSED]             	= { "RESCUER_TYPE_PARSED",             		"rtpa" },
	[RESCUER_DIGITAL_TWIN_ADDED]      	= { "RESCUER_DIGITAL_TWIN_ADDED",      		"rdta" },
	[EMERGENCY_PARSED]                	= { "EMERGENCY_PARSED",                		"empa" },
	[RESCUER_REQUEST_ADDED]           	= { "RESCUER_REQUEST_ADDED",           		"rrad" },
	[EMERGENCY_REQUEST_RECEIVED]      	= { "EMERGENCY_REQUEST_RECEIVED",      		"errr" },
	[EMERGENCY_REQUEST_PROCESSED]     	= { "EMERGENCY_REQUEST_PROCESSED",     		"erpr" },
	[MESSAGE_QUEUE]                   	= { "MESSAGE_QUEUE",                   		"mque" },
	[EMERGENCY_STATUS]                	= { "EMERGENCY_STATUS",                		"esta" },
	[RESCUER_STATUS]                  	= { "RESCUER_STATUS",                  		"rsta" },
	[EMERGENCY_REQUEST]               	= { "EMERGENCY_REQUEST",               		"erre" }
	
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
	mtx_lock(&log_mutex);

	if (!log_file) {
		perror("File di log non aperto");
		exit(EXIT_FAILURE);
	}

	long time = get_time();
	fprintf(
		log_file,
		LOG_EVENT_STRING, 
		time, get_log_event_type_code(e), 
		id, 
		get_log_event_type_string(e), 
		message
	);

	fflush(log_file);

	mtx_unlock(&log_mutex);
}
