#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 8080
#define MAX_CLIENTS 50
#define MAX_VOTERS 200
#define MAX_CANDIDATES 10
#define BUFFER_SIZE 1024

typedef struct {
    char name[30];
    int votes;
} Candidate;

typedef struct {
    Candidate candidates[MAX_CANDIDATES];
    int num_candidates;

    char voted_ids[MAX_VOTERS][50];
    int total_voters;

    bool is_open;
    pthread_mutex_t lock;
    FILE *log_file;
} ElectionState;

ElectionState election;

void init_election();
void setup_candidates();
void *client_handler(void *socket_desc);
void *admin_console_handler(void *arg);
void log_event(const char *event, const char *detail);
void generate_final_report();
bool has_voted(const char *id);

int main() {
    int server_sock, client_sock, *new_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t c_size;
    pthread_t admin_thread;

    init_election();
    setup_candidates();

    // Inicia thread admin
    if (pthread_create(&admin_thread, NULL, admin_console_handler, NULL) < 0) {
        perror("Erro ao criar thread de admin");
        return 1;
    }

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock == -1) { perror("Erro socket"); return 1; }

    int opt = 1;
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Erro bind"); return 1;
    }

    listen(server_sock, MAX_CLIENTS);

    printf("\n=== SERVIDOR ONLINE NA PORTA %d ===\n", PORT);
    printf("--> Digite 'CLOSE' neste terminal a qualquer momento para encerrar a votacao.\n\n");
    log_event("SYSTEM", "Server started");

    while ((client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &c_size))) {
        printf("Novo cliente conectado.\n");

        pthread_t sniffer_thread;
        new_sock = malloc(sizeof(int));
        *new_sock = client_sock;

        if (pthread_create(&sniffer_thread, NULL, client_handler, (void *)new_sock) < 0) {
            perror("Erro thread cliente");
            return 1;
        }
        pthread_detach(sniffer_thread);
    }

    fclose(election.log_file);
    return 0;
}

void setup_candidates() {
    int n;
    printf("=== CONFIGURACAO DA ELEICAO ===\n");
    do {
        printf("Quantos candidatos participarao (Max %d)? ", MAX_CANDIDATES);
        scanf("%d", &n);
    } while (n < 2 || n > MAX_CANDIDATES);

    election.num_candidates = n;
    getchar();

    for (int i = 0; i < n; i++) {
        printf("Nome do Candidato %d: ", i + 1);
        char name[30];
        fgets(name, 30, stdin);
        name[strcspn(name, "\n")] = 0;
        strcpy(election.candidates[i].name, name);
        election.candidates[i].votes = 0;
    }
    printf("Candidatos registrados com sucesso!\n");
}

void *admin_console_handler(void *arg) {
    char command[50];
    while (1) {
        scanf("%s", command);

        if (strcmp(command, "CLOSE") == 0) {
            pthread_mutex_lock(&election.lock);
            if (election.is_open) {
                election.is_open = false;
                generate_final_report();
                log_event("ADMIN", "Votacao encerrada via console local");
                printf("\n!!! VOTACAO ENCERRADA PELO ADMINISTRADOR !!!\n");
                printf("Relatorio gerado em 'resultado_final.txt'\n");
            } else {
                printf("A votacao ja esta encerrada.\n");
            }
            pthread_mutex_unlock(&election.lock);
        } else {
            printf("Comando desconhecido. Digite 'CLOSE' para encerrar.\n");
        }
    }
    return NULL;
}

void *client_handler(void *socket_desc) {
    int sock = *(int *)socket_desc;
    free(socket_desc);
    char buffer[BUFFER_SIZE], response[BUFFER_SIZE], voter_id[50] = "UNKNOWN";
    int read_size;

    while ((read_size = recv(sock, buffer, BUFFER_SIZE, 0)) > 0) {
        buffer[read_size] = '\0';
        buffer[strcspn(buffer, "\r\n")] = 0;

        char command[20], arg[50] = {0};
        sscanf(buffer, "%s %s", command, arg);

        if (strcmp(command, "HELLO") == 0) {
            strcpy(voter_id, arg);
            sprintf(response, "WELCOME %s\n", voter_id);
            send(sock, response, strlen(response), 0);
        }
        else if (strcmp(command, "LIST") == 0) {
            // Constroi string com todos os candidatos
            strcpy(response, "OPTIONS");
            char temp[50];
            sprintf(temp, " %d", election.num_candidates);
            strcat(response, temp);

            for(int i=0; i < election.num_candidates; i++) {
                strcat(response, " ");
                strcat(response, election.candidates[i].name);
            }
            strcat(response, "\n");
            send(sock, response, strlen(response), 0);
        }
        else if (strcmp(command, "VOTE") == 0) {
            pthread_mutex_lock(&election.lock);

            if (!election.is_open) {
                send(sock, "ERR CLOSED\n", 11, 0);
            }
            else if (has_voted(voter_id)) {
                send(sock, "ERR DUPLICATE\n", 14, 0);
            }
            else {
                int found = 0;
                for (int i = 0; i < election.num_candidates; i++) {
                    if (strcmp(election.candidates[i].name, arg) == 0) {
                        election.candidates[i].votes++;
                        strcpy(election.voted_ids[election.total_voters++], voter_id);
                        found = 1;
                        log_event("VOTE", voter_id);
                        break;
                    }
                }
                if (found) send(sock, "OK VOTED\n", 9, 0);
                else send(sock, "ERR INVALID_OPTION\n", 19, 0);
            }
            pthread_mutex_unlock(&election.lock);
        }
        else if (strcmp(command, "SCORE") == 0) {
            pthread_mutex_lock(&election.lock);
            strcpy(response, election.is_open ? "SCORE " : "CLOSED FINAL ");

            char temp[50];
            sprintf(temp, "%d", election.num_candidates);
            strcat(response, temp);

            for (int i = 0; i < election.num_candidates; i++) {
                sprintf(temp, " %s:%d", election.candidates[i].name, election.candidates[i].votes);
                strcat(response, temp);
            }
            strcat(response, "\n");
            pthread_mutex_unlock(&election.lock);
            send(sock, response, strlen(response), 0);
        }
        else if (strcmp(command, "BYE") == 0) {
            break;
        }
    }
    close(sock);
    return 0;
}

void init_election() {
    election.total_voters = 0;
    election.is_open = true;
    pthread_mutex_init(&election.lock, NULL);
    election.log_file = fopen("eleicao.log", "a");
}

bool has_voted(const char *id) {
    for (int i = 0; i < election.total_voters; i++) {
        if (strcmp(election.voted_ids[i], id) == 0) return true;
    }
    return false;
}

void log_event(const char *event, const char *detail) {
    if (election.log_file) {
        fprintf(election.log_file, "EVENT: %s | DET: %s\n", event, detail);
        fflush(election.log_file);
    }
}

void generate_final_report() {
    FILE *f = fopen("resultado_final.txt", "w");
    if (f) {
        fprintf(f, "=== RESULTADO FINAL ===\n");
        for(int i=0; i < election.num_candidates; i++) {
            fprintf(f, "%s: %d votos\n", election.candidates[i].name, election.candidates[i].votes);
        }
        fprintf(f, "Total de votantes: %d\n", election.total_voters);
        fclose(f);
    }
}
