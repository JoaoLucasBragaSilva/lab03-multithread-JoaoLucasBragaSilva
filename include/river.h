/**
 * @file river.h
 * @brief Definições para gerenciamento do rio e barcos
 * 
 * Este módulo implementa a lógica central da simulação, gerenciando
 * o estado do rio, alocação de barcos, embarque de passageiros e
 * manutenção das estatísticas e histórico de travessias.
 */

#ifndef RIVER_H
#define RIVER_H

#include "common.h"

/**
 * @struct BoatThreadArgs
 * @brief Argumentos passados para a thread de barco
 */
typedef struct {
    River *river;
    int boat_index;
} BoatThreadArgs;

/**
 * @brief Inicializa a estrutura do rio com os parâmetros iniciais
 * 
 * Configura mutexes, semáforos, barreiras e inicializa todos os
 * contadores e estruturas de dados para o início da simulação.
 * 
 * @param river Ponteiro para a estrutura do rio a ser inicializada
 * @param max_boats Número máximo de barcos simultâneos permitidos
 */
void river_init(River *river, int max_boats);

/**
 * @brief Realiza o embarque de um passageiro em um barco
 * 
 * Registra o passageiro no barco especificado e, se for o capitão,
 * marca-o como responsável pela travessia.
 * 
 * @param river Ponteiro para a estrutura do rio
 * @param passenger_id ID do passageiro a embarcar
 * @param boat_index Índice do barco onde o passageiro embarcará
 * @param is_captain Flag indicando se o passageiro será o capitão (1) ou não (0)
 */
void board(River *river, int passenger_id, int boat_index, int is_captain);

/**
 * @brief Exibe o estado atual da simulação no console
 * 
 * Mostra informações sobre passageiros aguardando, barcos ativos,
 * estatísticas de travessia e outras informações relevantes para
 * depuração e acompanhamento da simulação.
 * 
 * @param river Ponteiro para a estrutura do rio
 */
void show_current_state(River *river);

/**
 * @brief Função principal da thread de barco
 * 
 * Gerencia o ciclo de vida de um barco: aguarda passageiros,
 * realiza a travessia, registra o histórico e libera o barco
 * para reutilização.
 * 
 * @param args Ponteiro para BoatThreadArgs com os dados do barco
 * @return NULL ao finalizar a travessia
 */
void *boat_thread(void *args);

/**
 * @brief Aloca um barco disponível para uso
 * 
 * Procura por um barco inativo no array de barcos e o ativa
 * para iniciar uma nova travessia. Thread-safe.
 * 
 * @param river Ponteiro para a estrutura do rio
 * @return Índice do barco alocado, ou -1 se não houver barcos disponíveis
 */
int allocate_boat(River *river);

/**
 * @brief Libera um barco após a conclusão da travessia
 * 
 * Marca o barco como inativo, permitindo sua reutilização
 * em travessias futuras. Thread-safe.
 * 
 * @param river Ponteiro para a estrutura do rio
 * @param boat_index Índice do barco a ser liberado
 */
void release_boat(River *river, int boat_index);

/**
 * @brief Registra um novo passageiro no sistema
 * 
 * Inicializa os dados de um passageiro e o adiciona ao array
 * de passageiros do rio. Thread-safe.
 * 
 * @param river Ponteiro para a estrutura do rio
 * @param id Identificador único do passageiro
 * @param type Tipo do passageiro (HACKER ou SERF)
 */
void river_register_passenger(River *river, int id, PassengerType type);

/**
 * @brief Libera todos os recursos alocados pelo rio
 * 
 * Destroi mutexes, semáforos, barreiras e libera qualquer
 * memória alocada dinamicamente durante a simulação.
 * 
 * @param river Ponteiro para a estrutura do rio a ser destruída
 */
void river_destroy(River *river);

/**
 * @brief Adiciona uma mensagem formatada ao buffer de log do visualizador
 * 
 * Armazena mensagens em um buffer circular que será exibido pelo
 * visualizador. Suporta formatação estilo printf.
 * Thread-safe: protegida pelo mutex do rio.
 * 
 * @param river Ponteiro para a estrutura do rio
 * @param format String de formato estilo printf
 * @param ... Argumentos variáveis para a formatação
 */
void river_add_message(River *river, const char *format, ...);

#endif