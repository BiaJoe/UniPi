#include "emergency_priority_queue.h"




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
	n -> emergency = e;
	n -> prev = NULL;
	n -> next = NULL;
	return n;
}

// libera il singolo nodo ma non l'emergenza contenuta
void free_emergency_node(emergency_node_t* n){
	if(!n) return;
	free(n);
}

emergency_list_t *mallocate_emergency_list(){
	emergency_list_t *el = (emergency_list_t *)malloc(sizeof(emergency_list_t));	
	el -> head = NULL;
	el -> tail = NULL;
	el -> node_amount = 0;
	check_error_mtx_init(mtx_init(&(el->list_mutex), mtx_plain));
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
	mtx_destroy(&(list->list_mutex));
	free(list);
}

emergency_queue_t *mallocate_emergency_queue(){
	emergency_queue_t *q = (emergency_queue_t*)malloc(sizeof(emergency_queue_t));
	for(int i = MIN_EMERGENCY_PRIORITY; i <= MAX_EMERGENCY_PRIORITY; i++)
		q->lists[i] = mallocate_emergency_list();
	check_error_mtx_init(mtx_init(&(q->queue_mutex), mtx_plain));
	return q;
}

void free_emergency_queue(emergency_queue_t *q){
	for(int i = MIN_EMERGENCY_PRIORITY; i <= MAX_EMERGENCY_PRIORITY; i++)
		free_emergency_list(q->lists[i]);
	mtx_destroy(&(q->queue_mutex));
	free(q);
}

void append_emergency_node(emergency_list_t* list, emergency_node_t* node){
	if(!list || !node){											// se ho allocato un nodo da appendere in una lista insesistente
		if(node) free_emergency_node(node);		// e quindi non lo appendo finisco per perdere il riferimento a quel nodo per sempre
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

int is_the_first_node_of_the_list(emergency_node_t* node){
	return (node->prev == NULL) ? 1 : 0 ;
}

int is_the_last_node_of_the_list(emergency_node_t* node){
	return (node->next == NULL) ? 1 : 0 ;
}

void remove_emergency_node_from_its_list(emergency_node_t* node){
	
	if(!node || !(node->list)) return;								// se il nodo non è in nessuna lista non c'è niente da fare
			if (is_the_first_node_of_the_list(node)) {		// trattamento del primo nodo:
				node->list->head = node->next;							// - aggiorno la testa della lista
 				node->list->head->prev = NULL;							// - la testa ha NULL come precedente
			} else node->prev->next = node->next; 				// tutti gli altri collegano il precedente al successivo
			if (is_the_last_node_of_the_list(node)) {			// trattamento della coda del nodo:
				node->list->tail = node->prev;							// - aggiorno la coda della lista
				node->list->tail->next = NULL;							// - il next della coda è sempre NULL
			} else node->next->prev = node->prev;					// tutti i nodi tranne l'ultimo
		node->prev = NULL;															// annullo i puntatori del nodo 
		node->next = NULL;															// per non rischiare di accedervi quando è fuori
		node->list->node_amount -= 1;										// decremento il numero di nodi nella lista
}

// rimuove e ritorna il nodo testa
emergency_node_t* decapitate_emergency_list(emergency_list_t* list){
	if(!list || list->node_amount == 0) return NULL; // controllo la vuotezza
	emergency_node_t* head = list->head;
	remove_emergency_node_from_its_list(head);
	head -> prev = NULL;														// il nodo estratto non ha un prev
	head -> next = NULL;														// nè un next
	return head;
}

void enqueue_emergency(emergency_queue_t* q, emergency_t *e){
	int p = get_emergency_priority(e);
	if(p == INVALID_EMERGENCY_PROPERTY_NUMBER) return;		// misura di sicurezza, i controlli sono già stati fatti ma non si sa mai
	emergency_node_t* node = mallocate_emergency_node(e);	// creo il nuovo nodo
	append_emergency_node(q->lists[p], node);							// appendo il nodo alla lista
}

emergency_t* dequeue_emergency(emergency_queue_t* q){
	if(!q) return NULL;
	int p = MAX_EMERGENCY_PRIORITY;
	emergency_node_t *h = NULL; //head
	emergency_t *e = NULL;
	while(p >= MIN_EMERGENCY_PRIORITY){									// tento la decapitate ad ogni priorità dalla più grande alla più piccola
		emergency_list_t *current_list = q->lists[p--];		// dalla lista di maggiore priorità a quella di minore
		h = decapitate_emergency_list(current_list);			// decapitate non alloca memoria, non c'è il rischio di memory leak a riassegnare h
		if(h != NULL) break;															// se ho trovato un h valido vuol dire che ho decapitato quella lista con successo
	}
	e = h ? h->emergency : NULL;												// se la lista è vuota il risultato è NULL
	free_emergency_node(h);															// il nodo non mi serve più, adesso lavoro sull'emergenza!
	return e;					
}

void change_node_priority_list(emergency_queue_t* q, emergency_node_t* n, short p){
	remove_emergency_node_from_its_list(n);
	append_emergency_node(q->lists[p], n);
}

void promote_to_medium_priority_if_needed(emergency_queue_t* q, emergency_node_t* n){ // solo da 0 a 1 e non diversamente
	if(time(NULL) - get_emergency_time(n->emergency) >= MAX_TIME_IN_0_PRIORITY_BEFORE_PROMOTION)
		change_node_priority_list(q, n, MEDIUM_EMERGENCY_PRIORITY);
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