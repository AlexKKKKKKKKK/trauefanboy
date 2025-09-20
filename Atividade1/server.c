#include "proto.h"

// definir o número máx. de clients controlado pelo select
#define MAX_CLIENTS     FD_SETSIZE
#define BUF_SIZE        1024

static void die(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {

    if (argc != 2) {
        fprintf(stderr, "Use %s <porta>\nEx: %s 5001\n", argv[0], argv[0]);
        return EXIT_FAILURE;
    }

    int port = atoi(argv[1]);
    if (port <= 0 || port > 65535) {
        fprintf(stderr, "Porta inválida!!\n");
        return EXIT_FAILURE;
    }

    // criação do socket
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen < 0) die("socket");

    int yes = 1; // para habilitar o reuso da porta após o fechamento do servidor

    //o socket, level (nível de opção), otpname (reusar o não o endereço), optval, optlen
    if(setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0)
        die("setsockopt(SO_REUSEADDR)");


    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_port = htons((uint16_t)port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    // bind
    if (bind(listen_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
        die("bind");

    // listen
    if (listen(listen_fd, 8) < 0) //número máximo de conns pendentes
        die("listen");

    printf("\nServidor conectdo e ouvindo em 0.0.0.0: %d ...\n", port);

    // vetor de clientes para guards os FDs
    int clients[MAX_CLIENTS];

    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i] = -1; //aquele "slot" está livre
    }

    fd_set allset, rset;
    FD_ZERO(&allset);
    FD_SET(listen_fd, &allset);
    int maxfd = listen_fd;
    int max_i = -1;

    char buf[BUF_SIZE];

    for (;;) {
        rset = allset; // cópia (select modifica o set)
        // select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout)
        //  - nfds:     1 + maior descritor monitorado (maxfd + 1)
        //  - readfds:  conjunto de FDs para verificação de leitura pronta
        //  - writefds: (não usado aqui) NULL
        //  - exceptfds:(não usado) NULL
        //  - timeout:  (bloqueante) NULL para esperar indefinidamente
        int nready = select(maxfd + 1, &rset, NULL, NULL, NULL);
        if (nready < 0) {
            if (errno == EINTR) continue; // interrompido por sinal
            die("select");
        }

        // Novo cliente chegando?
        if (FD_ISSET(listen_fd, &rset)) {
            struct sockaddr_in cliaddr;
            socklen_t clilen = sizeof(cliaddr);
            // accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
            //  - sockfd: socket em listen
            //  - addr:   (saída) endereço do cliente conectado (pode ser NULL)
            //  - addrlen:(entrada/saída) tamanho do addr; ajustado pelo kernel
            int connfd = accept(listen_fd, (struct sockaddr*)&cliaddr, &clilen);
            if (connfd < 0) {
                perror("accept");
            } else {
                char ip[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &cliaddr.sin_addr, ip, sizeof(ip));
                printf("Novo cliente conectado %s:%d (fd=%d)\n", ip, ntohs(cliaddr.sin_port), connfd);

                // Adiciona na lista de clientes
                int i;
                for (i = 0; i < MAX_CLIENTS; i++) {
                    if (clients[i] < 0) {
                        clients[i] = connfd;
                        break;
                    }
                }
                if (i == MAX_CLIENTS) {
                    fprintf(stderr, "Muitos clientes, recusando.\n");
                    close(connfd);
                } else {
                    FD_SET(connfd, &allset);
                    if (connfd > maxfd) maxfd = connfd;
                    if (i > max_i) max_i = i;

                    const char *welcome = "Bem-vindo ao mini chat!\n";
                    send(connfd, welcome, strlen(welcome), 0);
                }
            }
            if (--nready <= 0) continue; // nada mais pronto
        }

        // Verifica dados vindos dos clientes existentes
        for (int i = 0; i <= max_i; i++) {
            int fd = clients[i];
            if (fd < 0) continue;
            if (FD_ISSET(fd, &rset)) {
                ssize_t n = recv(fd, buf, sizeof(buf) - 1, 0);
                // recv(int sockfd, void *buf, size_t len, int flags)
                //  - sockfd: socket do qual receber
                //  - buf:    buffer destino
                //  - len:    bytes máximos a ler
                //  - flags:  0 normal (sem flags)
                if (n <= 0) {
                    if (n < 0) perror("recv");
                    printf("Cliente fd=%d desconectou.\n", fd);
                    close(fd);
                    FD_CLR(fd, &allset);
                    clients[i] = -1;
                } else {
                    buf[n] = '\0'; // garantir string
                    if(buf[n-1]=='\n') buf[n-1] = '\0';
                    if(strcmp(buf,"HELP") == 0){
                        char help[50] = "DIGITE A CONTA NA FORMA INFIXA EX: 10+20/2\n";
                        send(fd,help,sizeof(help),0);
                    }
                    else if(strcmp(buf,"QUIT") == 0){
                        char quit[50] = "so apertar ctrl+d :)\n";
                        send(fd,quit,sizeof(quit),0);
                    }
                    else{
                        float num = calculate(buf);
                        if(num == FLT_MAX){
                            char erro[30] = "Erro string invalida\n";
                            send(fd,erro,sizeof(erro),0);
                        }
                        else{
                            char resp[BUF_SIZE];
                            snprintf(resp, sizeof(resp), "%f\n", num);
                            send(fd,resp,sizeof(resp),0);
                            // Também logar no servidor
                            printf("[MSG RECEBIDA fd=%d] %s", fd, buf);
                        }
                    }
                }
                if (--nready <= 0) break; // nenhum FD restante pronto
            }
        }
    }

    close(listen_fd);
    return 0;
}
