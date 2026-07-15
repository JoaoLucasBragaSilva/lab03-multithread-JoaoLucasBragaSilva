/**
 * @file passenger.c
 * @brief Implementação da lógica de sincronização dos passageiros
 */

#include "passenger.h"
#include "river.h"

/**
 * @brief Thread que implementa o comportamento de um passageiro
 * 
 * Implementa a máquina de estados para formação de grupos válidos:
 * - 4 hackers: formam um barco de hackers
 * - 4 serfs: formam um barco de serfs  
 * - 2 hackers + 2 serfs: formam um barco misto
 * 
 * O passageiro que detecta a condição de grupo completo torna-se
 * o capitão, responsável por alocar o barco e iniciar a travessia.
 * Utiliza semáforos para sincronizar a formação dos grupos.
 * 
 * @param args Ponteiro para PassengerArgs com id, tipo e referência ao rio
 * @return NULL após completar a travessia
 */
void *passenger_thread(void *args) {
    PassengerArgs *p_args = (PassengerArgs*) args;
    int id = p_args->id;
    River *river = p_args->river;
    PassengerType type = p_args->type;
    bool isCaptain = false;
    int boat_index = -1;
    
    river->passengers[id].waiting = 1;
    
    pthread_mutex_lock(&river->mutex);
    
    /* Incrementa contador do tipo correspondente */
    if (type == HACKER) {
        river->hackers++;
    } else {
        river->serfs++;
    }
    
    /* Verifica condições para formação de grupo completo */
    if (type == HACKER && river->hackers == 4) {
        /* Grupo de 4 hackers */
        sem_post(&river->hackerQueue);
        sem_post(&river->hackerQueue);
        sem_post(&river->hackerQueue);
        sem_post(&river->hackerQueue);
        river->hackers = 0;
        isCaptain = true;
        river_add_message(river, "Hacker %d: 4 hackers! Formando barco!", id);
    }
    else if (type == SERF && river->serfs == 4) {
        /* Grupo de 4 serfs */
        sem_post(&river->serfQueue);
        sem_post(&river->serfQueue);
        sem_post(&river->serfQueue);
        sem_post(&river->serfQueue);
        river->serfs = 0;
        isCaptain = true;
        river_add_message(river, "Serf %d: 4 serfs! Formando barco!", id);
    }
    else if (type == HACKER && river->hackers == 2 && river->serfs >= 2) {
        /* Grupo misto: 2 hackers + 2 serfs (iniciado por hacker) */
        sem_post(&river->hackerQueue);
        sem_post(&river->hackerQueue);
        sem_post(&river->serfQueue);
        sem_post(&river->serfQueue);
        river->serfs -= 2;
        river->hackers = 0;
        isCaptain = true;
        river_add_message(river, "Hacker %d: 2H+2S! Formando barco misto!", id);
    }
    else if (type == SERF && river->serfs == 2 && river->hackers >= 2) {
        /* Grupo misto: 2 serfs + 2 hackers (iniciado por serf) */
        sem_post(&river->serfQueue);
        sem_post(&river->serfQueue);
        sem_post(&river->hackerQueue);
        sem_post(&river->hackerQueue);
        river->hackers -= 2;
        river->serfs = 0;
        isCaptain = true;
        river_add_message(river, "Serf %d: 2S+2H! Formando barco misto!", id);
    }
    else {
        /* Condição não satisfeita, libera mutex e aguarda na fila */
        pthread_mutex_unlock(&river->mutex);
    }
    
    /* Aguarda na fila do semáforo correspondente ao seu tipo */
    if (type == HACKER) {
        sem_wait(&river->hackerQueue);
    } else {
        sem_wait(&river->serfQueue);
    }
    
    /* Capitão aloca o barco; passageiros comuns procuram barco ativo */
    if (isCaptain) {
        sem_wait(&river->boat_available);
        boat_index = allocate_boat(river);
    } else {
        pthread_mutex_lock(&river->mutex);
        
        for (int i = 0; i < MAX_BOATS; i++) {
            if (river->boats[i].active) {
                for (int j = 0; j < BOAT_CAPACITY; j++) {
                    if (river->boats[i].passengers[j] == -1) {
                        boat_index = i;
                        break;
                    }
                }
                if (boat_index >= 0) break;
            }
        }
    }
    
    board(river, id, boat_index, isCaptain ? 1 : 0);
    pthread_mutex_unlock(&river->mutex);
    
    /* Sincroniza todos os passageiros antes da partida */
    pthread_barrier_wait(&river->barrier);
    
    /* Capitão cria thread do barco para gerenciar a travessia */
    if (isCaptain) {
        BoatThreadArgs *bt_args = malloc(sizeof(BoatThreadArgs));
        bt_args->river = river;
        bt_args->boat_index = boat_index;
        
        pthread_t boat_tid;
        pthread_create(&boat_tid, NULL, boat_thread, bt_args);
        pthread_detach(boat_tid);
    }
    
    pthread_mutex_lock(&river->mutex);
    river->finished_threads++;
    pthread_mutex_unlock(&river->mutex);
    
    return NULL;
}