/**
 * @file visualizer.c
 * @brief Implementação da thread de visualização da simulação
 */

#include "visualizer.h"
#include "river.h"

/**
 * @brief Thread que exibe periodicamente o estado da simulação
 * 
 * Executa um loop que atualiza a visualização em intervalos regulares
 * enquanto a simulação estiver ativa. Utiliza o mutex do rio para
 * garantir acesso thread-safe ao estado compartilhado.
 * 
 * @param args Ponteiro para VisualizerArgs contendo referência ao rio e intervalo
 * @return NULL quando a simulação é encerrada
 */
void *visualizer_thread(void *args) {
    VisualizerArgs *vis_args = (VisualizerArgs*) args;
    River *river = vis_args->river;
    
    while (1) {
        pthread_mutex_lock(&river->mutex);
        
        int active = river->simulation_active;
        
        if (!active) {
            pthread_mutex_unlock(&river->mutex);
            break;
        }
        
        show_current_state(river);
        
        pthread_mutex_unlock(&river->mutex);
        
        usleep(vis_args->interval_ms * 1000);
    }
    
    return NULL;
}