# Tabella dei Thread per il Sistema di Gestione Emergenze

| Nome Thread             | File Implementazione             | Responsabilità                                                                                                     | Temporizzazione                                   | Sincronizzazione                                                                                                    |
| ----------------------- | -------------------------------- | ------------------------------------------------------------------------------------------------------------------ | ------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------- |
| **Main Thread**         | `src/server/main.c`              | - Inizializzazione<br>- Avvio altri thread<br>- Gestione segnali<br>- Terminazione ordinata                        | Nessuna (attesa attiva su segnali)                | - Invia segnali di terminazione agli altri thread<br>- Attende join di tutti i thread                               |
| **Message Receiver**    | `src/server/message_receiver.c`  | - Ricezione messaggi dalla coda<br>- Validazione messaggi<br>- Inserimento emergenze in coda interna               | Continua (bloccante su coda messaggi)             | - Mutex per accesso alla coda interna<br>- Condition variable per segnalare nuove emergenze                         |
| **Emergency Scheduler** | `src/server/scheduler.c`         | - Ordinamento emergenze per priorità<br>- Selezione emergenze da processare<br>- Assegnazione/prenotazione risorse | 500ms-1s                                          | - Mutex per accesso alle code di emergenze<br>- Read-write lock per accesso ai soccorritori disponibili             |
| **Aging Manager**       | `src/server/scheduler.c`         | - Incremento priorità emergenze in attesa<br>- Prevenzione starvation                                              | 5s                                                | - Read-write lock per accesso alle emergenze                                                                        |
| **Simulation Manager**  | `src/server/rescuer_manager.c`   | - Aggiornamento posizione soccorritori<br>- Calcolo tempi di arrivo<br>- Notifica arrivo sul posto                 | 200ms                                             | - Mutex per aggiornamento posizioni<br>- Condition variable per notificare arrivi                                   |
| **Timeout Monitor**     | `src/server/emergency_manager.c` | - Verifica timeouts delle emergenze<br>- Cambio stato emergenze scadute<br>- Logging eventi timeout                | 1s                                                | - Read lock per accesso alle emergenze<br>- Write lock per modificare stato emergenze                               |
| **Emergency Workers**   | `src/server/emergency_manager.c` | - Gestione completa di una singola emergenza<br>- Cambio stati<br>- Coordinamento soccorritori                     | Event-driven (risvegliato da condition variables) | - Mutex per accesso ai dati dell'emergenza<br>- Condition variables per eventi (arrivo soccorritori, completamento) |
| **Logger Thread**       | `src/server/logger.c`            | - Scrittura asincrona su file di log<br>- Formattazione messaggi di log                                            | Event-driven (coda interna di messaggi)           | - Mutex e condition variable per coda messaggi di log                                                               |

## Note Aggiuntive:

### Thread Pool per Emergency Workers

Per ottimizzare le risorse, è consigliabile implementare un thread pool di Emergency Workers anziché creare un thread per ogni emergenza:

- Dimensione consigliata: 4-8 thread (da configurare in base alle capacità del sistema)
- Ogni worker thread processa una emergenza alla volta dalla coda prioritaria

### Preemption (per chi non ha superato le prove in itinere)

Nel caso sia richiesta la preemption:

- Emergency Scheduler gestisce anche la sospensione di emergenze a bassa priorità
- Simulation Manager implementa la logica per interruzione e reindirizzamento dei soccorritori

### Thread Opzionali

Per sistemi più complessi o ottimizzazioni specifiche:

- **Statistics Collector**: raccoglie statistiche sul sistema (tempo medio risposta, etc.)
- **UI Thread**: se si implementa un'interfaccia di monitoraggio