#include "emergency_priority_queue.h"

// funzioni di gestione della coda di emergenze

emergency_t *mallocate_emergency(server_context_t *ctx, char* name, int x, int y, time_t timestamp){
	emergency_t *e = (emergency_t *)malloc(sizeof(emergency_t));
	check_error_memory_allocation(e);
	emergency_type_t *type = get_emergency_type_by_name(name, get_emergency_types_from_server_context(ctx));
	emergency_status_t status = WAITING;
	// calcolo il numero di rescuer totali
	// per ogni rescuer request dell'emergency_type 
	// ci sono required_count twins da aggiungere
	int rescuer_count = 0;
	for(int i = 0; i < type->rescuers_req_number; i++)
		rescuer_count += type->rescuers[i]->required_count;
	// alloco l'array di twins tutti a null perchè ancora non li ho cercati tra i twin disponibili
	// aggiungo 1 così è anche null-terminated
	rescuer_digital_twin_t **rescuer_twins = (rescuer_digital_twin_t **)calloc(rescuer_count + 1, sizeof(rescuer_digital_twin_t *));
	check_error_memory_allocation(rescuer_twins);

	e->type = type;
	e->status = status;
	e->x = x;
	e->y = y;
	e->time = timestamp;
	e->rescuer_count = rescuer_count;
	e->rescuer_twins = rescuer_twins;

	return e;
}

void free_emergency(emergency_t* e){
	if(!e) return;
	free(e->rescuer_twins);			// libero il puntatore ai gemelli digitali dei rescuers (non i gemelli stessi ovviamente)
	free(e);
}


emergency_node_t* mallocate_emergency_node(emergency_t *e){
	emergency_node_t* n = (emergency_node_t*)malloc(sizeof(emergency_node_t));
	n -> list = NULL;
	n -> priority = e->type->priority;	// prendo la priorità dall'emergency_type
	n -> rescuers_are_arriving = NO;
	n -> rescuers_have_arrived = NO;
	n -> asks_for_rescuers_from_lower_priorities = NO;
	n -> time_estimated_for_rescuers_to_arrive = UNDEFINED_TIME_FOR_RESCUERS_TO_ARRIVE; // inizialmente non so quanto ci metteranno i rescuers ad arrivare
	n -> emergency = e;
	n -> prev = NULL;
	n -> next = NULL;
	check_error_mtx_init(mtx_init(&(n->mutex), mtx_plain)); // inizializzo il mutex del nodo
	return n;
}

void free_emergency_node(emergency_node_t* n){
	if(!n) return;
	mtx_destroy(&(n->mutex));	// distruggo il mutex del nodo
	free(n);
}


emergency_list_t *mallocate_emergency_list(){
	emergency_list_t *el = (emergency_list_t *)malloc(sizeof(emergency_list_t));	
	el -> head = NULL;
	el -> tail = NULL;
	el -> node_amount = 0;
	check_error_mtx_init(mtx_init(&(el->mutex), mtx_plain));
	return el;
}

void free_emergency_list(emergency_list_t *list){
	if(!list) return;
		emergency_node_t *current = list->head;
		while (current != NULL) {									// percorro la lista da head a tail->next (=NULL) e libero un nodo alla volta
			emergency_node_t *next = current->next; // l'ultima volta next è null
			free_emergency_node(current);
			current = next;
		}
	mtx_destroy(&(list->mutex));
	free(list);
}


emergency_queue_t *mallocate_emergency_queue(){
	emergency_queue_t *q = (emergency_queue_t*)malloc(sizeof(emergency_queue_t));
	for(int i = MIN_EMERGENCY_PRIORITY; i <= MAX_EMERGENCY_PRIORITY; i++)
		q->lists[i] = mallocate_emergency_list();
	q->is_empty = YES; 																// inizialmente la coda è vuota
	check_error_cnd_init(cnd_init(&(q->not_empty)));	
	check_error_mtx_init(mtx_init(&(q->mutex), mtx_plain));
	return q;
}

int is_queue_empty(emergency_queue_t *q){
	return q-> is_empty;
}

void free_emergency_queue(emergency_queue_t *q){
	for(int i = MIN_EMERGENCY_PRIORITY; i <= MAX_EMERGENCY_PRIORITY; i++)
		free_emergency_list(q->lists[i]);
	cnd_destroy(&(q->not_empty));				// distruggo la condizione di notifica
	mtx_destroy(&(q->mutex));
	free(q);
}


void append_emergency_node(emergency_list_t* list, emergency_node_t* node){
	if(!list || !node){											// se ho allocato un nodo da appendere in una lista insesistente
		// if(node) free_emergency_node(node);		// e quindi non lo appendo finisco per perdere il riferimento a quel nodo per sempre
		return;																// per cui devo liberarlo e dimenticarmene
	}
	if (list->node_amount == 0){					// se la lista è vuota (caso base)
		list -> head = node;								// il nodo diventa sia capo che coda
		list -> tail = node;
		node -> list = list;
		node -> prev = NULL;
		node -> next = NULL;
	} else {
		list -> tail -> next = node; 				// il tail diventa il penultimo nodo che punta all'ultimo con next		
		node -> prev = list->tail;					// l'ultimo nodo punta il penultimo con prev
		node -> next = NULL; 								// il nodo è diventato la nuova coda
		node -> list = list;								// la lista di appartenenza del nodo diventa list
		list -> tail = node;								// l'ultimo nodo (tail) diventa ufficialmente node
	}			
	list->node_amount += 1;								// il numero di nodi aumenta di 1 !!!
}

void push_emergency_node(emergency_list_t* list, emergency_node_t *node){
	if(!list || !node){											// se ho allocato un nodo da appendere in una lista insesistente
		// if(node) free_emergency_node(node);		// e quindi non lo appendo finisco per perdere il riferimento a quel nodo per sempre
		return;																// per cui devo liberarlo e dimenticarmene
	}
	if (list->node_amount == 0){					// se la lista è vuota (caso base)
		list -> head = node;								// il nodo diventa sia capo che coda
		list -> tail = node;
		node -> list = list;
		node -> prev = NULL;
		node -> next = NULL;
	} else {
		list -> head -> prev = node;				// il nodo diventa il nuovo primo nodo
		node -> next = list->head;					// il nuovo nodo punta al vecchio primo nodo
		node -> prev = NULL;								// il nuovo nodo non ha un precedente perchè è la testa
		node -> list = list;								// la lista di appartenenza del nodo diventa list
		list -> head = node;								// il nuovo nodo diventa la nuova testa della lista
	}			
	list->node_amount += 1;								// il numero di nodi aumenta di 1 !!!
}


int is_the_first_node_of_the_list(emergency_node_t* node){
	return (node->prev == NULL) ? YES : NO ;
}

int is_the_last_node_of_the_list(emergency_node_t* node){
	return (node->next == NULL) ? YES : NO ;
}

void remove_emergency_node_from_its_list(emergency_node_t* node){
	if(!node || !(node->list)) return;											// se il nodo non è in nessuna lista non c'è niente da fare
			if (is_the_first_node_of_the_list(node)) {					// trattamento del primo nodo:
				node->list->head = node->next;										// - aggiorno la testa della lista
 				node->list->head->prev = NULL;										// - la testa ha NULL come precedente
			} else node->prev->next = node->next; 							// tutti gli altri collegano il precedente al successivo
			if (is_the_last_node_of_the_list(node)) {						// trattamento della coda del nodo:
				node->list->tail = node->prev;										// - aggiorno la coda della lista
				node->list->tail->next = NULL;										// - il next della coda è sempre NULL
			} else node->next->prev = node->prev;								// tutti i nodi tranne l'ultimo
		node->prev = NULL;																		// annullo i puntatori del nodo 
		node->next = NULL;																		// per non rischiare di accedervi quando è fuori
		node->list->node_amount -= 1;													// decremento il numero di nodi nella lista
}

emergency_node_t* decapitate_emergency_list(emergency_list_t* list){
	if(!list || list->node_amount == 0) return NULL;				// controllo la vuotezza
	emergency_node_t* head = list->head;				
	remove_emergency_node_from_its_list(head);				
	head -> prev = NULL;																		// il nodo estratto non ha un prev
	head -> next = NULL;																		// nè un next
	return head;
}

void enqueue_emergency_node(emergency_queue_t* q, emergency_node_t *n){
	if(!q || !n) return; 																		// se la coda o il nodo sono null non faccio nulla
	if(q->is_empty) q->is_empty = NO;												// se la coda era vuota, ora non lo è più
	int p = n->priority; 					
	append_emergency_node(q->lists[p], n);									// appendo il nodo alla lista
}


void enqueue_emergency(emergency_queue_t* q, emergency_t *e){
	emergency_node_t* n = mallocate_emergency_node(e);			// creo il nuovo nodo
	enqueue_emergency_node(q, n);														// appendo il nodo alla lista
}

emergency_node_t* dequeue_emergency_node(emergency_queue_t* q){
	if(!q) return NULL;
	int p = MAX_EMERGENCY_PRIORITY;
	emergency_node_t *h = NULL; 														// head
	while(p >= MIN_EMERGENCY_PRIORITY){											// tento la decapitate ad ogni priorità dalla più grande alla più piccola
		h = decapitate_emergency_list(q->lists[p--]);					
		if(h != NULL) break;																	// se ho trovato un h valido vuol dire che ho decapitato quella lista con successo
	}
	if(h == NULL) q->is_empty = YES;												// se non ho trovato nodi la lista deve essere vuota
	return h; 																							// ritorno il nodo decapitato	
}

void change_node_priority_list(emergency_queue_t* q, emergency_node_t* n, short newp){
	remove_emergency_node_from_its_list(n);
	n->priority = newp; 																		
	append_emergency_node(q->lists[newp], n);
}

int promote_to_medium_priority_if_needed(emergency_queue_t* q, emergency_node_t* n){ // solo da 0 a 1 e non diversamente
	if(time(NULL) - get_emergency_time(n->emergency) >= MAX_TIME_IN_MIN_PRIORITY_BEFORE_PROMOTION){
		change_node_priority_list(q, n, MEDIUM_EMERGENCY_PRIORITY);
		return YES;
	}
	return NO;
}

int timeout_waiting_emergency_if_needed(emergency_node_t* n){
	if(n->emergency->status == TIMEOUT) return NO;
	time_t current_time = time(NULL);
	time_t emergency_time = get_emergency_time(n->emergency);
	time_t elapsed_time = current_time - emergency_time;
	int priority = n->priority;
	
	if(
		priority == MAX_EMERGENCY_PRIORITY && elapsed_time > MAX_TIME_IN_MAX_PRIORITY_BEFORE_TIMEOUT ||
		priority == MEDIUM_EMERGENCY_PRIORITY && elapsed_time > MAX_TIME_IN_MEDIUM_PRIORITY_BEFORE_TIMEOUT
	) {
		n->emergency->status = TIMEOUT;
		return YES;
	};

	return NO;
}		


int get_emergency_priority(emergency_t* e){
	if(!e) return INVALID_EMERGENCY_PROPERTY_NUMBER;
	return (int) ((e -> type) -> priority);
}

int get_emergency_x(emergency_t* e){
	if(!e) return INVALID_EMERGENCY_PROPERTY_NUMBER;
	return e -> x ;
}

int get_emergency_y(emergency_t* e){
	if(!e) return INVALID_EMERGENCY_PROPERTY_NUMBER;
	return e -> y ;
}

time_t get_emergency_time(emergency_t* e){
	if(!e) return INVALID_EMERGENCY_PROPERTY_NUMBER;
	return (time_t) e -> time ;
}

emergency_status_t get_emergency_status(emergency_t* e){
	if(!e) return INVALID_EMERGENCY_PROPERTY_NUMBER;
	return (emergency_status_t) (e -> status) ;
}

int get_emergency_rescuer_req_number(emergency_t* e){
	if(!e) return INVALID_EMERGENCY_PROPERTY_NUMBER;
	return (int) ((e -> type) -> rescuers_req_number);
}

rescuer_request_t **get_emergency_rescuer_requests(emergency_t* e){
	if(!e) return INVALID_EMERGENCY_PROPERTY_POINTER;
	return (rescuer_request_t **) ((e -> type) -> rescuers);
}

void lock_queue(emergency_queue_t *q){
	LOCK(q->mutex);
}

void unlock_queue(emergency_queue_t *q){
	UNLOCK(q->mutex);
}

void lock_list(emergency_list_t *l){
	LOCK(l->mutex);
}

void unlock_list(emergency_list_t *l){
	UNLOCK(l->mutex);
}

void lock_node(emergency_node_t *n){
	if(n) LOCK(n->mutex);									// se il nodo è NULL non faccio il lock
}

void unlock_node(emergency_node_t *n){
	if(n) UNLOCK(n->mutex);
}

