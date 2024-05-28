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

int producao; // Dado do Servidor SE
int consumo;  // Dado do Servidor SCII

void usage(int argc, char **argv) {
    printf("usage: %s <v4|v6> <server port>\n", argv[0]);
    printf("example: %s v4 51511\n", argv[0]);
    exit(EXIT_FAILURE);
}

/**
 * @brief Gera valor aleatorio de Producao de Energia Eletrica = [20,50] MWh
 * 
 * @return int 
 */
int geraProducaoSE() {
    srand(time(NULL));
    return (rand() % 31) + 20;
}

/**
 * @brief Gera valor aleatorio de Consumo Elétrico = [20,100]%
 * 
 * @return int 
 */
int geraConsumoSCII() {
    srand(time(NULL));
    return (rand() % 101);
}

struct client_data {
    int csock;
    struct sockaddr_storage storage;
    int client_id; // ID do cliente
};


int client_count = 0;
int next_client_id = 1; // Proximo ID disponível
pthread_mutex_t client_count_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t client_id_mutex = PTHREAD_MUTEX_INITIALIZER;

void * client_thread_SE(void *data) {
    struct client_data *cdata = (struct client_data *)data;
    struct sockaddr *caddr = (struct sockaddr *)(&cdata->storage);

    char caddrstr[BUFSZ];
    addrtostr(caddr, caddrstr, BUFSZ);

    /* Mensagem de boas vindas para o cliente */ 
    char msg[BUFSZ];
    snprintf(msg, BUFSZ, "Servidor SE New ID: %d\n", cdata->client_id);
    size_t msg_len = strlen(msg);
    if (send(cdata->csock, msg, msg_len, 0) != msg_len) {
        perror("send");
    }

    while(1) {
        char buf[BUFSZ];
        memset(buf, 0, BUFSZ);
        size_t count = recv(cdata->csock, buf, BUFSZ - 1, 0);
        if (count == -1) {
            perror("recv");
            break; // Encerra a comunicação em caso de erro
        } else if (count == 0) {
            // Cliente desconectado
            printf("[log] Client disconnected: %s\n", caddrstr);
            break;
        }
        printf("[msg] %s, %d bytes: %s\n", caddrstr, (int)count, buf);

        if(strcmp(buf, "kill\n") == 0 ) {
            printf("Servidor SE Client %d removed\n", cdata->client_id);
            const char *response_kill = "Successful disconnect!\n";
            send(cdata->csock, response_kill, strlen(response_kill), 0);
            break;
        } else if(strcmp(buf, "display info se\n") == 0 ) {
            printf("REQ_INFOSE\n");
            printf("REQ_INFOSE %d\n", producao);
            char response_info[BUFSZ];
            snprintf(response_info, BUFSZ, "producao atual: %d kWh\n", producao);
            size_t response_info_len = strlen(response_info);
            send(cdata->csock, response_info, response_info_len, 0);
        }else if(strcmp(buf, "query condition\n") == 0) {
            printf("REQ_STATUS\n");
            if(producao >= 41) {
                const char *response_status_1 = "estado atual: alta!\n";
                send(cdata->csock, response_status_1, strlen(response_status_1), 0);
            } else if(producao >= 31){
                const char *response_status_2 = "estado atual: moderada!\n";
                send(cdata->csock, response_status_2, strlen(response_status_2), 0);
            } else {
                const char *response_status_3 = "estado atual: baixa!\n";
                send(cdata->csock, response_status_3, strlen(response_status_3), 0);
            }
        } else {
            const char *response_other = "Received your message!\n";
            send(cdata->csock, response_other, strlen(response_other), 0);
        }
    }
    close(cdata->csock);

    // Decrementa a contagem de clientes ao finalizar a conexão
    pthread_mutex_lock(&client_count_mutex);
    client_count--;
    pthread_mutex_unlock(&client_count_mutex);

    free(cdata);
    pthread_exit(EXIT_SUCCESS);
}


void * client_thread_SCII(void *data) {
    struct client_data *cdata = (struct client_data *)data;
    struct sockaddr *caddr = (struct sockaddr *)(&cdata->storage);

    char caddrstr[BUFSZ];
    addrtostr(caddr, caddrstr, BUFSZ);

    /* Mensagem de boas vindas para o cliente */ 
    char msg[BUFSZ];
    snprintf(msg, BUFSZ, "Servidor SCII New ID: %d\n", cdata->client_id);
    size_t msg_len = strlen(msg);
    if (send(cdata->csock, msg, msg_len, 0) != msg_len) {
        perror("send");
    }

    while(1) {
        char buf[BUFSZ];
        memset(buf, 0, BUFSZ);
        size_t count = recv(cdata->csock, buf, BUFSZ - 1, 0);
        if (count == -1) {
            perror("recv");
            break; // Encerra a comunicação em caso de erro
        } else if (count == 0) {
            // Cliente desconectado
            printf("[log] Client disconnected: %s\n", caddrstr);
            break;
        }
        printf("[msg] %s, %d bytes: %s\n", caddrstr, (int)count, buf);

        if(strcmp(buf, "kill\n") == 0 ) {
            printf("Servidor SCII Client %d removed\n", cdata->client_id);
            const char *response_kill = "Successful disconnect!\n";
            send(cdata->csock, response_kill, strlen(response_kill), 0);
            break;
        } else if(strcmp(buf, "display info scii\n") == 0 ) {
            printf("REQ_INFOSCII\n");
            printf("REQ_INFOSCII %d%%\n", consumo);
            char response_info[BUFSZ];
            snprintf(response_info, BUFSZ, "producao atual: %d%% kWh\n", consumo);
            size_t response_info_len = strlen(response_info);
            send(cdata->csock, response_info, response_info_len, 0);
        } else {
            const char *response_others = "Received your message!\n";
            send(cdata->csock, response_others, strlen(response_others), 0);
        }
    }
    close(cdata->csock);

    // Decrementa a contagem de clientes ao finalizar a conexão
    pthread_mutex_lock(&client_count_mutex);
    client_count--;
    pthread_mutex_unlock(&client_count_mutex);

    free(cdata);
    pthread_exit(EXIT_SUCCESS);
}

int main(int argc, char **argv) {
    /* Inicializando servidor */
    if (argc < 3) {
        usage(argc, argv);
    }
    
    // Atribuindo dados de Consumo e Producao dos servidores
    consumo = geraConsumoSCII();
    producao = geraProducaoSE(); 

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
            const char *response = "Client limit exceeded\n";
            send(csock, response, strlen(response), 0);
            close(csock);
            printf("Client limit exceeded\n");
        } else {
            client_count++;
            struct client_data *cdata = malloc(sizeof(*cdata));
            if (!cdata) {
                logexit("malloc");
            }
            cdata->csock = csock;
            memcpy(&(cdata->storage), &cstorage, sizeof(cstorage));

            // Atribui um ID único ao cliente
            pthread_mutex_lock(&client_id_mutex);
            cdata->client_id = next_client_id++;
            pthread_mutex_unlock(&client_id_mutex);

            printf("Client %d added\n", cdata->client_id);

            pthread_t tid;
            if (atoi(argv[2]) == 12345) {
                pthread_create(&tid, NULL, client_thread_SE, cdata);
                pthread_detach(tid);
            }
            if (atoi(argv[2]) == 54321) {
                pthread_create(&tid, NULL, client_thread_SCII, cdata);
                pthread_detach(tid);
            }
        }
        pthread_mutex_unlock(&client_count_mutex);
    }
    
    pthread_mutex_destroy(&client_count_mutex);
    pthread_mutex_destroy(&client_id_mutex);
    exit(EXIT_SUCCESS);
}