/**
 * @file common.h
 * @brief Definições comuns e estruturas de dados para simulação de travessia de rio
 * 
 * Este arquivo contém as definições de tipos, estruturas e constantes
 * utilizadas na simulação do problema de sincronização de hackers e serfs
 * atravessando um rio em barcos com capacidade limitada.
 */

#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <stdbool.h>

/* Constantes de configuração do sistema */
#define MAX_THREADS     100
#define BOAT_CAPACITY   4
#define MAX_BOATS       10
#define MAX_HISTORY     50
#define MAX_MESSAGES    100
#define MSG_LENGTH      256

/**
 * @enum PassengerType
 * @brief Enumeração dos tipos de passageiros
 */
typedef enum {
    HACKER,
    SERF
} PassengerType;

/**
 * @struct Passenger
 * @brief Estrutura que representa um passageiro no sistema
 */
typedef struct {
    int id;
    PassengerType type;
    int boarded;
    int crossed;
    int waiting;
} Passenger;

/**
 * @struct Boat
 * @brief Estrutura que representa um barco no sistema
 */
typedef struct {
    int boat_id;
    int passengers[BOAT_CAPACITY];
    int captain_index;
    int position;
    int crossing;
    int active;
} Boat;

/**
 * @struct CrossingRecord
 * @brief Estrutura para registrar o histórico de travessias
 */
typedef struct {
    int boat_id;
    int passengers[BOAT_CAPACITY];
    PassengerType passenger_types[BOAT_CAPACITY];
    int captain_id;
    int num_passengers;
} CrossingRecord;

/**
 * @struct River
 * @brief Estrutura principal que gerencia todo o estado da simulação
 */
typedef struct {
    int hackers;
    int serfs;
    
    /* Gerenciamento de barcos */
    Boat boats[MAX_BOATS];
    int active_boats;
    int next_boat_id;
    int max_boats;
    
    /* Mecanismos de sincronização */
    pthread_mutex_t mutex;
    pthread_barrier_t barrier;
    sem_t hackerQueue;
    sem_t serfQueue;
    sem_t boat_available;
    
    /* Sistema de logging circular */
    char messages[MAX_MESSAGES][MSG_LENGTH];
    int message_count;
    int message_head;
    
    /* Controle de timeout */
    time_t last_boat_time;
    int timeout_seconds;
    int force_stop;
    
    /* Estado da simulação */
    int simulation_active;
    int total_threads;
    int finished_threads;
    int total_created;
    Passenger passengers[MAX_THREADS];
    
    /* Estatísticas da simulação */
    int total_boats;
    int total_crossed;
    int total_hackers_crossed;
    int total_serfs_crossed;
    
    /* Histórico de travessias */
    CrossingRecord history[MAX_HISTORY];
    int history_count;
    
} River;

#endif