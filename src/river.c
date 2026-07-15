/**
 * @file river.c
 * @brief Implementação das funções de gerenciamento do rio e barcos
 */

#include "river.h"
#include <stdarg.h>

/**
 * @brief Adiciona mensagem formatada ao buffer de log circular
 * 
 * Armazena mensagens no buffer do visualizador usando o mesmo formato
 * do printf. O buffer é circular mas a função message_count continua
 * incrementando para manter o histórico total.
 * 
 * @param river Ponteiro para a estrutura do rio
 * @param format String de formato estilo printf
 * @param ... Argumentos variáveis conforme o formato
 */
void river_add_message(River *river, const char *format, ...) {
    if (river->message_count < MAX_MESSAGES) {
        va_list args;
        va_start(args, format);
        vsnprintf(river->messages[river->message_count], MSG_LENGTH, format, args);
        va_end(args);
        river->message_count++;
    }
}

/**
 * @brief Inicializa completamente a estrutura do rio
 * 
 * Configura todos os campos da estrutura River com valores padrão,
 * inicializa mutex, semáforos, barreira, arrays de passageiros,
 * barcos e histórico. Prepara o sistema para início da simulação.
 * 
 * @param river Ponteiro para estrutura River não inicializada
 * @param max_boats Número máximo de barcos simultâneos
 */
void river_init(River *river, int max_boats) {
    river->hackers = 0;
    river->serfs = 0;
    river->active_boats = 0;
    river->next_boat_id = 0;
    river->max_boats = max_boats;
    river->total_boats = 0;
    river->total_crossed = 0;
    river->total_hackers_crossed = 0;
    river->total_serfs_crossed = 0;
    river->simulation_active = 1;
    river->total_threads = 0;
    river->finished_threads = 0;
    river->total_created = 0;
    river->force_stop = 0;
    river->timeout_seconds = 15;
    river->last_boat_time = time(NULL);
    river->history_count = 0;
    river->message_count = 0;
    river->message_head = 0;
    
    /* Inicializa array de barcos como inativos */
    for (int i = 0; i < MAX_BOATS; i++) {
        river->boats[i].boat_id = i;
        river->boats[i].position = 0;
        river->boats[i].crossing = 0;
        river->boats[i].active = 0;
        river->boats[i].captain_index = -1;
        for (int j = 0; j < BOAT_CAPACITY; j++) {
            river->boats[i].passengers[j] = -1;
        }
    }
    
    /* Inicializa array de passageiros */
    for (int i = 0; i < MAX_THREADS; i++) {
        river->passengers[i].id = i;
        river->passengers[i].type = HACKER;
        river->passengers[i].boarded = 0;
        river->passengers[i].crossed = 0;
        river->passengers[i].waiting = 0;
    }
    
    /* Inicializa histórico de travessias vazio */
    for (int i = 0; i < MAX_HISTORY; i++) {
        river->history[i].boat_id = -1;
        river->history[i].num_passengers = 0;
        river->history[i].captain_id = -1;
        for (int j = 0; j < BOAT_CAPACITY; j++) {
            river->history[i].passengers[j] = -1;
        }
    }
    
    /* Inicializa buffer de mensagens vazio */
    for (int i = 0; i < MAX_MESSAGES; i++) {
        river->messages[i][0] = '\0';
    }
    
    /* Inicializa mecanismos de sincronização */
    pthread_mutex_init(&river->mutex, NULL);
    pthread_barrier_init(&river->barrier, NULL, BOAT_CAPACITY);
    sem_init(&river->hackerQueue, 0, 0);
    sem_init(&river->serfQueue, 0, 0);
    sem_init(&river->boat_available, 0, max_boats);
}

/**
 * @brief Aloca um barco inativo para nova travessia
 * 
 * Procura no array de barcos por um que não esteja ativo,
 * inicializa-o para uma nova travessia e atualiza o timestamp
 * da última partida. Deve ser chamada com o mutex do rio já adquirido.
 * 
 * @param river Ponteiro para a estrutura do rio
 * @return Índice do barco alocado, ou -1 se todos estiverem ocupados
 */
int allocate_boat(River *river) {
    for (int i = 0; i < river->max_boats; i++) {
        if (!river->boats[i].active) {
            river->boats[i].active = 1;
            river->boats[i].position = 0;
            river->boats[i].crossing = 0;
            river->boats[i].captain_index = -1;
            river->active_boats++;
            
            for (int j = 0; j < BOAT_CAPACITY; j++) {
                river->boats[i].passengers[j] = -1;
            }
            
            river->last_boat_time = time(NULL);
            river_add_message(river, "Barco %d alocado para nova travessia", i);
            
            return i;
        }
    }
    return -1;
}

/**
 * @brief Libera um barco após conclusão da travessia
 * 
 * Marca o barco como inativo, limpa seus dados e sinaliza
 * no semáforo que há um barco disponível para nova travessia.
 * Deve ser chamada com o mutex do rio já adquirido.
 * 
 * @param river Ponteiro para a estrutura do rio
 * @param boat_index Índice do barco a ser liberado
 */
void release_boat(River *river, int boat_index) {
    if (boat_index >= 0 && boat_index < MAX_BOATS) {
        int boat_id = river->boats[boat_index].boat_id;
        river->boats[boat_index].active = 0;
        river->boats[boat_index].crossing = 0;
        river->boats[boat_index].position = 0;
        river->boats[boat_index].captain_index = -1;
        river->active_boats--;
        
        for (int j = 0; j < BOAT_CAPACITY; j++) {
            river->boats[boat_index].passengers[j] = -1;
        }
        
        sem_post(&river->boat_available);
        river_add_message(river, "Barco %d liberado para reutilizacao", boat_id);
    }
}

/**
 * @brief Embarca um passageiro em um barco específico
 * 
 * Procura o primeiro assento vazio no barco indicado e registra
 * o passageiro. Se for capitão, marca o índice correspondente.
 * Atualiza os flags do passageiro para indicar que embarcou.
 * 
 * @param river Ponteiro para a estrutura do rio
 * @param passenger_id ID do passageiro a embarcar
 * @param boat_index Índice do barco de destino
 * @param is_captain Flag indicando se este passageiro é o capitão (1) ou não (0)
 */
void board(River *river, int passenger_id, int boat_index, int is_captain) {
    Boat *boat = &river->boats[boat_index];
    
    for (int i = 0; i < BOAT_CAPACITY; i++) {
        if (boat->passengers[i] == -1) {
            boat->passengers[i] = passenger_id;
            river->passengers[passenger_id].boarded = 1;
            river->passengers[passenger_id].waiting = 0;
            
            if (is_captain) {
                boat->captain_index = i;
                river_add_message(river, "%s %d embarcou no barco %d [CAPITAO]",
                       river->passengers[passenger_id].type == HACKER ? "Hacker" : "Serf",
                       passenger_id, boat->boat_id);
            } else {
                river_add_message(river, "%s %d embarcou no barco %d",
                       river->passengers[passenger_id].type == HACKER ? "Hacker" : "Serf",
                       passenger_id, boat->boat_id);
            }
            
            return;
        }
    }
}

/**
 * @brief Exibe o estado completo da simulação no console
 * 
 * Limpa a tela e mostra uma interface com:
 * - Últimos eventos registrados
 * - Passageiros aguardando nas margens
 * - Barcos em carregamento e travessia
 * - Estatísticas de travessias realizadas
 * - Passageiros que já atravessaram
 * 
 * Utiliza caracteres ANSI para limpeza de tela e formatação.
 * Deve ser chamada com o mutex do rio já adquirido.
 * 
 * @param river Ponteiro para a estrutura do rio
 */
void show_current_state(River *river) {
    int i;
    
    printf("\033[2J\033[H");
    
    printf("================================================================\n");
    printf("            TRAVESSIA DO RIO - REDMOND, WASHINGTON              \n");
    printf("================================================================\n");
    
    if (river->message_head < river->message_count) {
        printf("\n  ULTIMOS EVENTOS:\n");
        int show = river->message_count - river->message_head;
        if (show > 5) show = 5;
        
        for (i = river->message_head; i < river->message_head + show; i++) {
            if (i < river->message_count) {
                printf("  > %s\n", river->messages[i]);
            }
        }
        river->message_head += show;
        printf("\n  --------------------------------------------------------\n");
    }
    
    printf("\n");
    printf("  Margem Esquerda (Esperando):\n");
    printf("  Hackers: %-3d  |  Serfs: %-3d\n", river->hackers, river->serfs);
    
    printf("\n  Filas: ");
    int waiting_count = 0;
    for (i = 0; i < MAX_THREADS; i++) {
        if (river->passengers[i].waiting) {
            printf("%s%d ", 
                   river->passengers[i].type == HACKER ? "H" : "S", i);
            waiting_count++;
        }
    }
    if (waiting_count == 0) printf("[vazia]");
    printf("\n");
    
    printf("\n  --------------------------------------------------------\n");
    
    printf("\n  BARCOS PARTINDO:\n");
    
    int boats_crossing = 0;
    for (i = 0; i < MAX_BOATS; i++) {
        if (river->boats[i].active && river->boats[i].crossing) {
            Boat *b = &river->boats[i];
            printf("\n  Barco %d partindo com: [", b->boat_id);
            for (int j = 0; j < BOAT_CAPACITY; j++) {
                if (b->passengers[j] >= 0) {
                    printf("%s%d", 
                           river->passengers[b->passengers[j]].type == HACKER ? "H" : "S",
                           b->passengers[j]);
                    if (j == b->captain_index) {
                        printf("*");
                    } else {
                        printf(" ");
                    }
                } else {
                    printf("__ ");
                }
                if (j < BOAT_CAPACITY - 1) printf("|");
            }
            printf("]\n");
            
            if (b->captain_index >= 0 && b->passengers[b->captain_index] >= 0) {
                int cap_id = b->passengers[b->captain_index];
                printf("    Capitao: %s%d\n", 
                       river->passengers[cap_id].type == HACKER ? "Hacker " : "Serf   ",
                       cap_id);
            }
            boats_crossing++;
        }
    }
    
    int boats_loading = 0;
    for (i = 0; i < MAX_BOATS; i++) {
        if (river->boats[i].active && !river->boats[i].crossing) {
            printf("  Barco %d carregando na margem\n", river->boats[i].boat_id);
            boats_loading++;
        }
    }
    
    if (boats_crossing == 0 && boats_loading == 0) {
        printf("\n  [nenhum barco ativo]\n");
    }
    
    printf("\n  --------------------------------------------------------\n");
    
    printf("\n  Margem Direita (Atravessaram):\n");
    printf("  Total: %-3d  |  Hackers: %-3d  |  Serfs: %-3d  |  Travessias: %-3d\n",
           river->total_crossed, 
           river->total_hackers_crossed, 
           river->total_serfs_crossed,
           river->total_boats);
    
    printf("\n  Ja atravessaram: ");
    int crossed_count = 0;
    for (i = 0; i < MAX_THREADS; i++) {
        if (river->passengers[i].crossed) {
            printf("%s%d ", 
                   river->passengers[i].type == HACKER ? "H" : "S", i);
            crossed_count++;
        }
    }
    if (crossed_count == 0) printf("[nenhum ainda]");
    printf("\n");
    
    printf("\n  Barcos ativos: %d (max: %d)\n", river->active_boats, river->max_boats);
    
    printf("\n  Regras: [OK] 4H | [OK] 4S | [OK] 2H+2S | [X] 1H+3S | [X] 3H+1S\n");
    printf("================================================================\n");
    fflush(stdout);
}

/**
 * @brief Thread que gerencia a travessia de um barco
 * 
 * Executa o ciclo completo de travessia:
 * 1. Marca o barco como em travessia e exibe o estado
 * 2. Aguarda 3 segundos (simulando a travessia)
 * 3. Registra a travessia no histórico
 * 4. Atualiza estatísticas dos passageiros
 * 5. Libera o barco para reutilização
 * 
 * A thread é criada como detached pelo capitão e se auto-libera
 * após conclusão.
 * 
 * @param args Ponteiro para BoatThreadArgs com referência ao rio e índice do barco
 * @return NULL após conclusão da travessia
 */
void *boat_thread(void *args) {
    BoatThreadArgs *bt_args = (BoatThreadArgs*) args;
    River *river = bt_args->river;
    int boat_index = bt_args->boat_index;
    
    pthread_mutex_lock(&river->mutex);
    river->boats[boat_index].crossing = 1;
    river_add_message(river, "Barco %d partindo!", river->boats[boat_index].boat_id);
    show_current_state(river);
    pthread_mutex_unlock(&river->mutex);
    
    sleep(3);
    
    pthread_mutex_lock(&river->mutex);
    
    Boat *boat = &river->boats[boat_index];
    
    /* Registra travessia no histórico se houver espaço */
    if (river->history_count < MAX_HISTORY) {
        CrossingRecord *rec = &river->history[river->history_count];
        rec->boat_id = boat->boat_id;
        rec->num_passengers = 0;
        
        if (boat->captain_index >= 0 && boat->passengers[boat->captain_index] >= 0) {
            rec->captain_id = boat->passengers[boat->captain_index];
        } else {
            rec->captain_id = -1;
        }
        
        for (int i = 0; i < BOAT_CAPACITY; i++) {
            int pid = boat->passengers[i];
            if (pid >= 0) {
                rec->passengers[i] = pid;
                rec->passenger_types[i] = river->passengers[pid].type;
                rec->num_passengers++;
            } else {
                rec->passengers[i] = -1;
            }
        }
        
        river->history_count++;
    }
    
    /* Atualiza estatísticas dos passageiros que atravessaram */
    for (int i = 0; i < BOAT_CAPACITY; i++) {
        int pid = boat->passengers[i];
        if (pid >= 0) {
            river->passengers[pid].crossed = 1;
            river->passengers[pid].boarded = 0;
            river->total_crossed++;
            
            if (river->passengers[pid].type == HACKER) {
                river->total_hackers_crossed++;
            } else {
                river->total_serfs_crossed++;
            }
        }
    }
    
    river->total_boats++;
    river_add_message(river, "Barco %d chegou ao destino! Passageiros desembarcaram.", boat->boat_id);
    release_boat(river, boat_index);
    
    show_current_state(river);
    pthread_mutex_unlock(&river->mutex);
    
    free(bt_args);
    return NULL;
}

/**
 * @brief Registra um novo passageiro que chegou à margem do rio
 * 
 * Inicializa ou atualiza os dados do passageiro no array global
 * e marca-o como aguardando. Incrementa o contador de threads ativas.
 * Thread-safe: protegida pelo mutex do rio.
 * 
 * @param river Ponteiro para a estrutura do rio
 * @param id Identificador único do passageiro
 * @param type Tipo do passageiro (HACKER ou SERF)
 */
void river_register_passenger(River *river, int id, PassengerType type) {
    pthread_mutex_lock(&river->mutex);
    river->passengers[id].id = id;
    river->passengers[id].type = type;
    river->passengers[id].boarded = 0;
    river->passengers[id].crossed = 0;
    river->passengers[id].waiting = 1;
    river->total_threads++;
    river_add_message(river, "%s %d chegou na margem do rio",
           type == HACKER ? "Hacker" : "Serf", id);
    pthread_mutex_unlock(&river->mutex);
}

/**
 * @brief Libera todos os recursos de sincronização do rio
 * 
 * Destroi mutex, barreira e semáforos alocados durante river_init.
 * Deve ser chamada ao final da simulação para evitar vazamento de recursos.
 * 
 * @param river Ponteiro para a estrutura do rio a ser destruída
 */
void river_destroy(River *river) {
    pthread_mutex_destroy(&river->mutex);
    pthread_barrier_destroy(&river->barrier);
    sem_destroy(&river->hackerQueue);
    sem_destroy(&river->serfQueue);
    sem_destroy(&river->boat_available);
}