/**
 * @file main.c
 * @brief Programa principal da simulação de travessia do rio
 * 
 * Implementa a interface com usuário para configuração da simulação,
 * cria as threads de passageiros, gerencia o ciclo de vida da simulação
 * e exibe estatísticas finais.
 */

#include "common.h"
#include "river.h"
#include "passenger.h"
#include "visualizer.h"

/**
 * @brief Lê um número inteiro do usuário com validação de intervalo
 * 
 * Exibe uma mensagem solicitando um valor numérico e valida se
 * está dentro do intervalo [min, max]. Repete até obter entrada válida.
 * 
 * @param mensagem Texto descritivo a ser exibido ao usuário
 * @param min Valor mínimo aceito (inclusivo)
 * @param max Valor máximo aceito (inclusivo)
 * @return Número inteiro validado dentro do intervalo especificado
 */
int ler_numero(const char *mensagem, int min, int max) {
    int valor;
    char buffer[100];
    
    while (1) {
        printf("%s (%d a %d): ", mensagem, min, max);
        fflush(stdout);
        
        if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
            printf("Erro na leitura. Tente novamente.\n");
            continue;
        }
        
        if (sscanf(buffer, "%d", &valor) != 1) {
            printf("Entrada invalida. Digite um numero.\n");
            continue;
        }
        
        if (valor < min || valor > max) {
            printf("Valor fora do intervalo (%d a %d).\n", min, max);
            continue;
        }
        
        return valor;
    }
}

/**
 * @brief Exibe o relatório final da simulação
 * 
 * Apresenta um resumo completo com:
 * - Configuração utilizada
 * - Estatísticas de travessias
 * - Histórico detalhado de cada travessia
 * - Lista de passageiros que não conseguiram atravessar (se houver)
 * 
 * @param river Ponteiro para a estrutura do rio com dados acumulados
 * @param num_hackers Número total de hackers configurados
 * @param num_serfs Número total de serfs configurados
 * @param max_boats Número máximo de barcos configurado
 * @param intervalo_max Intervalo máximo entre chegadas configurado
 */
void exibir_estatisticas_finais(River *river, int num_hackers, int num_serfs, 
                                 int max_boats, int intervalo_max) {
    int total = num_hackers + num_serfs;
    int i, j;
    
    printf("\033[2J\033[H");
    printf("\n");
    printf("================================================================\n");
    printf("              TRAVESSIA COMPLETA - ESTATISTICAS FINAIS            \n");
    printf("================================================================\n");
    printf("\n");
    printf("  CONFIGURACAO:\n");
    printf("  Hackers: %d | Serfs: %d | Total: %d\n", num_hackers, num_serfs, total);
    printf("  Barcos simultaneos: %d | Intervalo max: %d ms\n", max_boats, intervalo_max);
    printf("\n");
    printf("----------------------------------------------------------------\n");
    printf("\n");
    printf("  RESULTADOS:\n");
    printf("  Travessias realizadas: %d\n", river->total_boats);
    printf("  Total atravessados: %d (Hackers: %d | Serfs: %d)\n", 
           river->total_crossed, river->total_hackers_crossed, river->total_serfs_crossed);
    printf("\n");
    
    if (river->history_count > 0) {
        printf("----------------------------------------------------------------\n");
        printf("\n");
        printf("  HISTORICO DE TRAVESSIAS:\n\n");
        
        for (i = 0; i < river->history_count; i++) {
            CrossingRecord *rec = &river->history[i];
            
            int h = 0, s = 0;
            for (j = 0; j < rec->num_passengers; j++) {
                if (rec->passenger_types[j] == HACKER) h++;
                else s++;
            }
            
            printf("  Travessia %d - Barco %d (%d Hackers + %d Serfs):\n", 
                   i + 1, rec->boat_id, h, s);
            printf("    Passageiros: [");
            for (j = 0; j < rec->num_passengers; j++) {
                printf("%s%d", 
                       rec->passenger_types[j] == HACKER ? "H" : "S",
                       rec->passengers[j]);
                if (rec->passengers[j] == rec->captain_id) {
                    printf("*");
                }
                if (j < rec->num_passengers - 1) printf(" | ");
            }
            printf("]\n");
            
            if (rec->captain_id >= 0) {
                printf("    Capitao: %s%d\n",
                       river->passengers[rec->captain_id].type == HACKER ? "Hacker " : "Serf   ",
                       rec->captain_id);
            }
            printf("\n");
        }
    }
    
    int restantes = total - river->total_crossed;
    if (restantes > 0) {
        printf("----------------------------------------------------------------\n");
        printf("\n");
        printf("  ATENCAO: %d passageiro(s) NAO atravessaram:\n", restantes);
        printf("  ");
        for (i = 0; i < MAX_THREADS; i++) {
            if (!river->passengers[i].crossed && 
                (river->passengers[i].waiting || river->passengers[i].boarded)) {
                printf("%s%d ", 
                       river->passengers[i].type == HACKER ? "H" : "S", i);
            }
        }
        printf("\n");
        printf("\n  Motivo: A combinacao restante e proibida pelas regras\n");
        printf("  de seguranca (1H+3S ou 3H+1S nao sao permitidas).\n");
    } else {
        printf("----------------------------------------------------------------\n");
        printf("\n");
        printf("  Todos os passageiros atravessaram com sucesso!\n");
    }
    
    printf("\n================================================================\n\n");
    fflush(stdout);
}

/**
 * @brief Função principal - ponto de entrada do programa
 * 
 * Fluxo de execução:
 * 1. Exibe regras e solicita configuração ao usuário
 * 2. Inicializa o rio e cria thread do visualizador
 * 3. Cria threads de passageiros em intervalos aleatórios
 * 4. Monitora condições de término (todos atravessaram ou timeout)
 * 5. Finaliza a simulação e exibe estatísticas
 * 
 * @param argc Número de argumentos da linha de comando (não utilizado)
 * @param argv Array de argumentos da linha de comando (não utilizado)
 * @return 0 em caso de sucesso, 1 se configuração inválida
 */
int main(int argc, char *argv[]) {
    srand(time(NULL));
    
    printf("\033[2J\033[H");
    printf("================================================================\n");
    printf("         TRAVESSIA DO RIO - REDMOND, WASHINGTON                 \n");
    printf("================================================================\n\n");
    
    printf("Regras de seguranca para o barco:\n");
    printf("  [OK] 4 Hackers\n");
    printf("  [OK] 4 Serfs\n");
    printf("  [OK] 2 Hackers + 2 Serfs\n");
    printf("  [PROIBIDO] 1 Hacker + 3 Serfs\n");
    printf("  [PROIBIDO] 3 Hackers + 1 Serf\n\n");
    
    printf("================================================================\n");
    printf("                   CONFIGURACAO DA SIMULACAO                      \n");
    printf("================================================================\n\n");
    
    int num_hackers = ler_numero("Quantidade de Hackers", 0, 50);
    int num_serfs = ler_numero("Quantidade de Serfs", 0, 50);
    
    int total = num_hackers + num_serfs;
    if (total < 4) {
        printf("\nERRO: E necessario pelo menos 4 pessoas no total!\n");
        return 1;
    }
    
    printf("\n");
    int max_boats = ler_numero("Numero maximo de barcos simultaneos", 1, 10);
    
    int intervalo_max = ler_numero(
        "Intervalo maximo entre chegadas (ms) [aleatorio entre 50 e este valor]",
        50, 5000
    );
    
    printf("\n================================================================\n");
    printf("                     INICIANDO SIMULACAO                         \n");
    printf("================================================================\n\n");
    printf("  Hackers: %d | Serfs: %d | Total: %d\n", num_hackers, num_serfs, total);
    printf("  Barcos simultaneos: %d\n", max_boats);
    printf("  Intervalo: aleatorio entre 50ms e %dms\n", intervalo_max);
    printf("  Timeout: 15s apos ultimo barco\n\n");
    printf("Iniciando em 3 segundos...\n");
    fflush(stdout);
    sleep(3);
    
    River river;
    river_init(&river, max_boats);
    
    pthread_t *threads = malloc(total * sizeof(pthread_t));
    PassengerArgs *p_args = malloc(total * sizeof(PassengerArgs));
    
    /* Cria thread do visualizador e aguarda inicialização */
    pthread_t visualizer;
    VisualizerArgs vis_args = {&river, 5000};
    pthread_create(&visualizer, NULL, visualizer_thread, &vis_args);
    usleep(500000);
    
    int h_criados = 0, s_criados = 0;
    
    /* Cria threads de passageiros em ordem aleatória */
    for (int i = 0; i < total; i++) {
        int criar_hacker;
        
        if (h_criados < num_hackers && s_criados < num_serfs) {
            criar_hacker = (rand() % 2 == 0);
        } else if (h_criados < num_hackers) {
            criar_hacker = 1;
        } else {
            criar_hacker = 0;
        }
        
        if (criar_hacker) {
            p_args[i].id = h_criados;
            p_args[i].river = &river;
            p_args[i].type = HACKER;
            river_register_passenger(&river, h_criados, HACKER);
            pthread_create(&threads[i], NULL, passenger_thread, &p_args[i]);
            h_criados++;
        } else {
            int s_id = s_criados + num_hackers;
            p_args[i].id = s_id;
            p_args[i].river = &river;
            p_args[i].type = SERF;
            river_register_passenger(&river, s_id, SERF);
            pthread_create(&threads[i], NULL, passenger_thread, &p_args[i]);
            s_criados++;
        }
        
        pthread_mutex_lock(&river.mutex);
        river.total_created++;
        pthread_mutex_unlock(&river.mutex);
        
        int intervalo_aleatorio = 50 + (rand() % (intervalo_max - 50 + 1));
        usleep(intervalo_aleatorio * 1000);
    }
    
    /* Loop de monitoramento das condições de término */
    while (1) {
        sleep(1);
        
        pthread_mutex_lock(&river.mutex);
        int active_boats = river.active_boats;
        int total_crossed = river.total_crossed;
        time_t last_boat = river.last_boat_time;
        pthread_mutex_unlock(&river.mutex);
        
        /* Todos atravessaram e não há barcos em trânsito */
        if (total_crossed >= total && active_boats == 0) {
            printf("\n[Sistema] Todos atravessaram! Encerrando...\n");
            fflush(stdout);
            break;
        }
        
        /* Timeout: 15 segundos sem atividade de barcos */
        time_t agora = time(NULL);
        if (difftime(agora, last_boat) >= 15 && active_boats == 0) {
            printf("\n[Sistema] Timeout de 15 segundos atingido! Encerrando...\n");
            fflush(stdout);
            break;
        }
    }
    
    /* Finaliza simulação e aguarda thread do visualizador */
    pthread_mutex_lock(&river.mutex);
    river.simulation_active = 0;
    pthread_mutex_unlock(&river.mutex);
    
    usleep(100000);
    pthread_join(visualizer, NULL);
    
    exibir_estatisticas_finais(&river, num_hackers, num_serfs, max_boats, intervalo_max);
    
    free(threads);
    free(p_args);
    
    river_destroy(&river);
    
    return 0;
}