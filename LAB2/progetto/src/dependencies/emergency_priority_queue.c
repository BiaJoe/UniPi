#include "emergency_priority_queue.h"







int parse_emergency_request(char *message, char* name, int *x, int *y, time_t *timestamp){
	if(sscanf(message, EMERGENCY_REQUEST_SYNTAX, name, x, y, timestamp) != 4)
		return 0;

	return 1;
}

int are_emergency_request_values_illegal(server_context_t *ctx, char* name, int x, int y, time_t timestamp){
	if(strlen(name) <= 0) return YES;
	int h = get_server_height(ctx);
	int w = get_server_width(ctx);
	if(!get_emergency_type_by_name(name, get_emergency_types_from_server_context(ctx))) return YES;
	if(ABS(x) < MIN_X_COORDINATE_ABSOLUTE_VALUE || ABS(x) > ABS(w)) return YES;
	if(ABS(y) < MIN_Y_COORDINATE_ABSOLUTE_VALUE || ABS(y) > ABS(h)) return YES;
	if(timestamp == INVALID_TIME) return YES;

	return NO;
}

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
	// libero il puntatore ai gemelli digitali dei rescuers (non i gemelli stessi ovviamente)
	free(e->rescuer_twins);
	free(e);
}

emergency_node_t* mallocate_emergency_node(emergency_t *e, emergency_node_t* prev, emergency_node_t* next){
	emergency_node_t* n = (emergency_node_t*)malloc(sizeof(emergency_node_t));
	n -> list = NULL;
	n -> emergency = e;
	n -> prev = prev;
	n -> next = next;
	check_error_mtx_init(mtx_init(&(n->node_mutex), mtx_plain));
	return n;
}

// libera il singolo nodo ma non l'emergenza contenuta
void free_emergency_node(emergency_node_t* n){
	if(!n) return;
	mtx_destroy(&(n->node_mutex));
	free(n);
}

emergency_list_t *mallocate_emergency_list(){
	emergency_list_t *el = (emergency_list_t *)malloc(sizeof(emergency_list_t));	
	el -> head = NULL;
	el -> tail = NULL;
	el -> nodes_amount = 0;
	check_error_mtx_init(mtx_init(&(el->list_mutex), mtx_plain));
	return el;
}

void free_emergency_list(emergency_list_t *list){
	if(!list) return;

	LOCK(list->list_mutex);

	// percorro la lista da head a tail->next (=NULL) e libero un nodo alla volta
	emergency_node_t *current = list->head;
	while (current != NULL) {
		emergency_node_t *next = current->next; // l'ultima volta next è null
		free_emergency_node(current);
		current = next;
	}
	// tutti i nodi sono liberi
	UNLOCK(list->list_mutex);
	mtx_destroy(&list->list_mutex);
	free(list);
}

void append_emergency_node(emergency_list_t* list, emergency_node_t* node){
	
	if(!list || !node) return;

	// locko la lista per non rischiare di fare la stessa operazione con altri thread
	LOCK(list->list_mutex);

		// se la lista è vuota (caso base)
		if (list->node_amount == 0){
			list->head = node;
			list->tail = node;
			LOCK(node->node_mutex);
				node -> list = list;
				node -> prev = NULL;
				node -> next = NULL;
			UNLOCK(node->node_mutex);
		} else {
			// il tail diventa il penultimo nodo che punta all'ultimo con next
			LOCK_UNLOCK_1(
				list -> tail -> node_mutex, 				
				list -> tail -> next = node; 				
			);

			// l'ultimo nodo punta il penultimo con prev
			// la lista di appartenenza del nodo diventa list
			// il nodo è diventato la nuova coda
			LOCK(node->node_mutex);
				node -> prev = list->tail;
				node -> next = NULL; 
				node -> list = list;
			UNLOCK(node->node_mutex);

			// l'ultimo nodo (tail) diventa ufficialmente node
			list -> tail = node;
		}

		// il numero di nodi aumenta di 1 !!!
		list->node_amount += 1;

	UNLOCK(list->list_mutex);
}

int is_the_first_node_of_the_list(emergency_node_t* node){
	return (node->prev == NULL) ? 1 : 0 ;
}

int is_the_last_node_of_the_list(emergency_node_t* node){
	return (node->next == NULL) ? 1 : 0 ;
}

void remove_emergency_node_from_its_list(emergency_node_t* node){
	// se il nodo non è in nessuna lista non c'è niente da fare
	if(!node->list) return;

	LOCK(node->list->list_mutex);
		LOCK(node->node_mutex);

			// se sono al primo nodo devo aggiornare head della lista
			if(is_the_first_node_of_the_list(node)) 
				node->list->head = node->next;		
			else // tutti i nodi tranne il primo
				node->prev->next = node->next; 
				
			// se sono all'ultimo devo aggoirnare tail
			if(is_the_last_node_of_the_list(node)) 
				node->list->tail = node->prev;
			else // tutti i nodi tranne l'ultimo
				node->next->prev = node->prev;
			
			// annullo i puntatori del nodo per non rischiare di accedervi quando è fuori
			node->prev = NULL;
			node->next = NULL;

			// funziona sicuramente per tutti i nodi, dimostrabile per induzione
			// nel caso base di un nodo singolo head = tail = NULL
		UNLOCK(node->node_mutex);

		// decremento il numero di nodi nella lista
		node->list->node_amount -= 1;

	UNLOCK(node->list->list_mutex);
}

// rimuove e ritorna il nodo testa
emergency_node_t* decapitate_emergency_list(emergency_list_t* list){

	if(!list || list->node_amount == 0) return NULL;
	emergency_node_t* head = list->head;

	LOCK(list->list_mutex);
		// il ruolo di testa passa al prossimo nodo (che può anche essere NULL)
		list -> head = head -> next; 
		LOCK( list->head->node_mutex);
			// head deve avere sempre il precedente nullo
			list -> head -> prev = NULL; 
		UNLOCK( list->head->node_mutex);
	UNLOCK(list->list_mutex);

	return head;
}

void enqueue_emergency(emergency_queue_t* eq, emergency_t *e){
	int p = get_emergency_priority(e);
	// DA FINIRE DOMANI SPERO DI FAREE TUTTO
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

rescuer_request_t **get_emergency_resquer_requests(emergency_t* e){
	if(!e) return INVALID_EMERGENCY_PROPERTY_POINTER;
	return (rescuer_request_t **) ((e -> type) -> rescuers);
}