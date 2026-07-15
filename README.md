# 🚣 River Crossing Problem - Travessia do Rio

Implementação multithread do problema clássico de sincronização "River Crossing" 
do livro **"The Little Book of Semaphores"** (Cap. 5.7) de Allen B. Downey.

## 📖 O Problema

Em Redmond, Washington, hackers Linux e funcionários da Microsoft ("serfs") 
compartilham um barco para atravessar um rio. O barco tem capacidade para 
**exatamente 4 pessoas** e restrições de segurança para evitar conflitos:

| Combinação | Permitido? |
|------------|:----------:|
| 4 Hackers | ✅ Sim |
| 4 Serfs | ✅ Sim |
| 2 Hackers + 2 Serfs | ✅ Sim |
| 1 Hacker + 3 Serfs | ❌ Não |
| 3 Hackers + 1 Serf | ❌ Não |

As regras existem porque grupos desbalanceados podem resultar em "acidentes" 
durante a travessia — um hacker sozinho com três serfs (ou vice-versa) 
provavelmente não chegaria ao outro lado.

## 🚀 Diferencial: Múltiplos Barcos Simultâneos

Diferente da implementação tradicional do livro (que usa apenas 1 barco por vez), 
este projeto permite configurar **vários barcos atravessando ao mesmo tempo**, 
controlados por um semáforo que gerencia a disponibilidade da frota.

## 📁 Estrutura do Projeto
├── common.h # Tipos, estruturas e constantes compartilhadas
├── river.h # Interface do gerenciador do rio
├── river.c # Implementação: barcos, embarque, estatísticas
├── passenger.h # Interface da thread de passageiro
├── passenger.c # Implementação: formação de grupos, capitão
├── visualizer.h # Interface do visualizador em tempo real
├── visualizer.c # Implementação: exibição do estado da simulação
├── main.c # Interface com usuário e loop principal
├── Makefile # Regras de compilação
└── README.md # Esta documentação


## ⚙️ Mecanismos de Sincronização

| Mecanismo | Escopo | Função |
|-----------|--------|--------|
| `pthread_mutex_t mutex` | Rio | Protege todo o estado compartilhado (contadores, barcos, estatísticas) |
| `sem_t hackerQueue` | Rio | Fila de hackers aguardando formação de grupo válido |
| `sem_t serfQueue` | Rio | Fila de serfs aguardando formação de grupo válido |
| `sem_t boat_available` | Rio | Controla o número máximo de barcos simultâneos em operação |
| `pthread_barrier_t barrier` | Rio | Sincroniza os 4 passageiros antes da partida do barco |

### Fluxo de Sincronização

1. Passageiros chegam e incrementam contadores (`hackers`/`serfs`)
2. Ao atingir uma combinação válida, o passageiro que detectou torna-se **capitão**
3. O capitão sinaliza os semáforos das filas para liberar os outros 3 passageiros
4. Passageiros são desbloqueados e procuram o barco alocado
5. Todos sincronizam na barreira antes da partida
6. O capitão cria uma thread detached para gerenciar a travessia

## 🎮 Funcionalidades

### Configuração Interativa

O programa solicita ao usuário:
- **Número de hackers** (0-50)
- **Número de serfs** (0-50)
- **Máximo de barcos simultâneos** (1-10)
- **Intervalo máximo entre chegadas** em milissegundos (50-5000)

Todas as entradas são validadas com limites e tratamento de erros.

### Visualização em Tempo Real

Uma thread dedicada exibe o estado da simulação a cada 5 segundos:
- Passageiros aguardando em cada fila
- Barcos carregando na margem
- Barcos em travessia com sua composição
- Capitão identificado com asterisco (`H3*`)
- Estatísticas acumuladas de travessias
- Últimos eventos registrados em buffer circular

### Sistema de Timeout

- Encerra automaticamente após **15 segundos** sem atividade de barcos
- Essencial para evitar deadlocks quando sobram combinações inválidas
  - Exemplo: restam 3 hackers e 1 serf (combinação proibida)

### Histórico de Travessias

- Registra até **50 travessias** com detalhes completos
- Cada registro armazena:
  - ID do barco
  - IDs e tipos dos passageiros
  - ID do capitão
  - Composição do grupo (ex: 2 Hackers + 2 Serfs)

### Relatório Final

Ao término, exibe:
- Configuração utilizada
- Total de travessias realizadas
- Passageiros que atravessaram (por tipo)
- Histórico detalhado de cada travessia
- Lista de passageiros que não conseguiram atravessar (se houver)
- Motivo do bloqueio (combinação proibida pelas regras)

## 🔨 Compilação e Execução

### Requisitos

- GCC com suporte a C11 (`-std=c11`)
- Biblioteca `pthread` (nativa em sistemas Linux/Unix)
- Terminal com suporte a sequências de escape ANSI
- Sistema operacional Linux/Unix (ou WSL no Windows)

### Comandos

```bash
make          # Compila o projeto
make run      # Compila (se necessário) e executa
make clean    # Remove binários e objetos

📊 Exemplo de Execução

================================================================
         TRAVESSIA DO RIO - REDMOND, WASHINGTON                 
================================================================

Regras de seguranca para o barco:
  [OK] 4 Hackers
  [OK] 4 Serfs
  [OK] 2 Hackers + 2 Serfs
  [PROIBIDO] 1 Hacker + 3 Serfs
  [PROIBIDO] 3 Hackers + 1 Serf

================================================================
                   CONFIGURACAO DA SIMULACAO                      
================================================================

Quantidade de Hackers (0 a 50): 6
Quantidade de Serfs (0 a 50): 6

Numero maximo de barcos simultaneos (1 a 10): 2

Intervalo maximo entre chegadas (ms) [aleatorio entre 50 e este valor] (50 a 5000): 1500

Saída Esperada (Visualização)

================================================================
            TRAVESSIA DO RIO - REDMOND, WASHINGTON              
================================================================

  ULTIMOS EVENTOS:
  > Hacker 0 chegou na margem do rio
  > Serf 6 chegou na margem do rio
  > Hacker 0: 2H+2S! Formando barco misto!
  > Hacker 0 embarcou no barco 0 [CAPITAO]
  > Barco 0 partindo!

  Margem Esquerda (Esperando):
  Hackers: 0   |  Serfs: 0

  BARCOS PARTINDO:
  Barco 0 partindo com: [H0*|S6 |H2 |S8 ]
    Capitao: Hacker 0

  Margem Direita (Atravessaram):
  Total: 4   |  Hackers: 2   |  Serfs: 2   |  Travessias: 1
================================================================

📚 Referência
Downey, Allen B. "The Little Book of Semaphores" — Capítulo 5.7: River Crossing Problem

Green Tea Press, 2ª Edição (2016)

Disponível gratuitamente em: https://greenteapress.com/semaphores/

📄 Licença
Este projeto é um exercício acadêmico de programação concorrente, implementado
como estudo dos mecanismos de sincronização POSIX.