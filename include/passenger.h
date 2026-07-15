/**
 * @file passenger.h
 * @brief Definições para a thread de passageiro na simulação do rio
 * 
 * Este módulo implementa o comportamento dos passageiros (hackers e serfs)
 * que desejam atravessar o rio, incluindo as regras de formação de grupos
 * e sincronização para embarque nos barcos.
 */

#ifndef PASSENGER_H
#define PASSENGER_H

#include "common.h"

/**
 * @struct PassengerArgs
 * @brief Argumentos passados para a thread de passageiro
 */
typedef struct {
    int id;
    River *river;
    PassengerType type;
} PassengerArgs;

/**
 * @brief Função principal da thread de passageiro
 * 
 * Cada passageiro tenta formar um grupo válido para atravessar o rio,
 * seguindo as regras: 4 hackers, 4 serfs, ou 2 hackers + 2 serfs.
 * Um dos passageiros atua como capitão do barco.
 * 
 * @param args Ponteiro para PassengerArgs com os dados do passageiro
 * @return NULL ao finalizar a travessia
 */
void *passenger_thread(void *args);

#endif