#include "common.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/types.h>

#include <time.h>

#define BUFSZ 1024

#define MAX_CLIENTS   10
#define SERVIDOR_SE   0
#define SERVIDOR_SCII 1

void usage(int argc, char **argv) {
    printf("usage: %s <v4|v6> <server port>\n", argv[0]);
    printf("example: %s v4 51511\n", argv[0]);
    exit(EXIT_FAILURE);
}

struct client_data {
    int csock;
    struct sockaddr_storage storage;
};

int client_count = 0;
pthread_mutex_t client_count_mutex = PTHREAD_MUTEX_INITIALIZER;

void * client_thread(void *data) {
    struct client_data *cdata = (struct client_data *)data;
    struct sockaddr *caddr = (struct sockaddr *)(&cdata->storage);

    char caddrstr[BUFSZ];
    addrtostr(caddr, caddrstr, BUFSZ);
    printf("[log] connection from %s\n", caddrstr);

    char buf[BUFSZ];
    memset(buf, 0, BUFSZ);
    size_t count = recv(cdata->csock, buf, BUFSZ - 1, 0);
    printf("[msg] %s, %d bytes: %s\n", caddrstr, (int)count, buf);

    sprintf(buf, "remote endpoint: %.1000s\n", caddrstr);
    count = send(cdata->csock, buf, strlen(buf) + 1, 0);
    if (count != strlen(buf) + 1) {
        logexit("send");
    }
    close(cdata->csock);

    // Decrementa a contagem de clientes ao finalizar a conexão
    pthread_mutex_lock(&client_count_mutex);
    client_count--;
    pthread_mutex_unlock(&client_count_mutex);

    free(cdata);
    pthread_exit(EXIT_SUCCESS);
}

/**
 * @brief Gera valor aleatorio de Producao de Energia Eletrica = [20,50] MWh
 * 
 * @return int 
 */
int geraProducaoSE();

/**
 * @brief Gera valor aleatorio de Consumo Elétrico = [20,100]%
 * 
 * @return int 
 */
int geraConsumoSCII();


int main(int argc, char **argv) {
    /* Inicializando servidor */
    if (argc < 3) {
        usage(argc, argv);
    }

    // Definindo servidor SE ou SCII
    int tipoServidor;
    int dadoArmazenado;
    if (atoi(argv[2]) == 12345) {
        dadoArmazenado = geraProducaoSE();
        tipoServidor = SERVIDOR_SE;
    } else {
        dadoArmazenado = geraConsumoSCII();
        tipoServidor = SERVIDOR_SCII;
    }

    struct sockaddr_storage storage;
    if (0 != server_sockaddr_init(argv[1], argv[2], &storage)) {
        usage(argc, argv);
    }

    int s;
    s = socket(storage.ss_family, SOCK_STREAM, 0);
    if (s == -1) {
        logexit("socket");
    }

    int enable = 1;
    if (0 != setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int))) {
        logexit("setsockopt");
    }

    struct sockaddr *addr = (struct sockaddr *)(&storage);
    if (0 != bind(s, addr, sizeof(storage))) {
        logexit("bind");
    }

    if (0 != listen(s, 10)) {
        logexit("listen");
    }

    char addrstr[BUFSZ];
    addrtostr(addr, addrstr, BUFSZ);
    printf("bound to %s, waiting connections\n", addrstr);

    while (1) {
        struct sockaddr_storage cstorage;
        struct sockaddr *caddr = (struct sockaddr *)(&cstorage);
        socklen_t caddrlen = sizeof(cstorage);

        int csock = accept(s, caddr, &caddrlen);
        if (csock == -1) {
            logexit("accept");
        }

        // Bloqueia o mutex antes de acessar e modificar a contagem de clientes
        pthread_mutex_lock(&client_count_mutex);
        if (client_count >= MAX_CLIENTS) {
            close(csock);
            printf("Número máximo de clientes atingido. Conexão recusada\n");
        } else {
            client_count++;
            struct client_data *cdata = malloc(sizeof(*cdata));
            if (!cdata) {
                logexit("malloc");
            }
            cdata->csock = csock;
            memcpy(&(cdata->storage), &cstorage, sizeof(cstorage));

            pthread_t tid;
            pthread_create(&tid, NULL, client_thread, cdata);
            pthread_detach(tid); // Detach da thread para que seus recursos sejam liberados automaticamente ao término
        }
        pthread_mutex_unlock(&client_count_mutex);
    }
    
    pthread_mutex_destroy(&client_count_mutex);  // Destroi o mutex ao finalizar o programa
    exit(EXIT_SUCCESS);
}





int geraProducaoSE() {
    int min = 20;
    int max = 50;

    int producao = rand() % (max - min + 1) + min;
    return producao;
}

int geraConsumoSCII() {
    int min = 0;
    int max = 100;

    int consumo = rand() % (max - min + 1);
    return consumo;
}