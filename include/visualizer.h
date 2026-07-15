/**
 * @file visualizer.h
 * @brief Definições para o visualizador da simulação do rio
 * 
 * Este módulo implementa uma thread de visualização que exibe
 * periodicamente o estado atual da simulação, incluindo as
 * margens do rio, barcos em travessia e estatísticas.
 */

#ifndef VISUALIZER_H
#define VISUALIZER_H

#include "common.h"

/**
 * @struct VisualizerArgs
 * @brief Argumentos passados para a thread do visualizador
 */
typedef struct {
    River *river;
    int interval_ms;
} VisualizerArgs;

/**
 * @brief Função principal da thread do visualizador
 * 
 * Executa um loop que periodicamente limpa a tela e exibe
 * o estado atual da simulação, incluindo passageiros nas margens,
 * barcos em travessia, estatísticas e mensagens de log.
 * A thread executa enquanto a simulação estiver ativa.
 * 
 * @param args Ponteiro para VisualizerArgs com os parâmetros de visualização
 * @return NULL quando a simulação for encerrada
 */
void *visualizer_thread(void *args);

#endif