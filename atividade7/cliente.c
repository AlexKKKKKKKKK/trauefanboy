#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024

void print_menu() {
    printf("\n--- MENU ---\n");
    printf("1. Listar Candidatos (LIST)\n");
    printf("2. Votar (VOTE)\n");
    printf("3. Ver Placar (SCORE)\n");
    printf("4. Sair (BYE)\n");
    printf("Escolha: ");
}

int main(int argc, char const *argv[]) {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};
    char voter_id[50];
    char message[BUFFER_SIZE];

    if (argc < 2) {
        printf("Uso: ./client <VOTER_ID>\n");
        return -1;
    }
    strcpy(voter_id, argv[1]);

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Erro socket \n"); return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("\n Endereco invalido \n"); return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\n Falha conexao \n"); return -1;
    }

    sprintf(message, "HELLO %s", voter_id);
    send(sock, message, strlen(message), 0);
    recv(sock, buffer, BUFFER_SIZE, 0);
    printf("Status: %s\n", buffer);

    int choice;
    char input_aux[50];

    while (1) {
        print_menu();
        if (scanf("%d", &choice) != 1) break;
        getchar();

        memset(message, 0, BUFFER_SIZE);
        memset(buffer, 0, BUFFER_SIZE);

        switch (choice) {
            case 1:
                strcpy(message, "LIST");
                break;
            case 2:
                printf("Nome do candidato: ");
                fgets(input_aux, 50, stdin);
                input_aux[strcspn(input_aux, "\n")] = 0;
                sprintf(message, "VOTE %s", input_aux);
                break;
            case 3:
                strcpy(message, "SCORE");
                break;
            case 4:
                strcpy(message, "BYE");
                send(sock, message, strlen(message), 0);
                close(sock);
                exit(0);
            default:
                printf("Opcao invalida.\n");
                continue;
        }

        send(sock, message, strlen(message), 0);
        int valread = recv(sock, buffer, BUFFER_SIZE, 0);

        if (valread > 0) {
            printf("\n>>> SERVER: %s\n", buffer);
        } else {
            printf("Conexao perdida.\n");
            break;
        }
    }
    return 0;
}
