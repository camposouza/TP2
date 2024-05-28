#include "common.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <stdbool.h>

#define BUFSZ 1024

void usage(int argc, char **argv) {
	printf("usage: %s <server IP> <server port 1> <server port 2>\n", argv[0]);
	printf("example: %s 127.0.0.1 12345 54321\n", argv[0]);
	exit(EXIT_FAILURE);
}

/**
 * @brief Obtem a primeira palavra de um buffer de caracteres e escreve em outro buffer
 * 
 * @param buffer  buffer de caracteres original
 * @param primeira buffer a ser escrito
 */
void primeira_palavra(const char *buffer, char *primeira) {

    char *copia_buffer = strdup(buffer);
    if (copia_buffer == NULL) {
        perror("strdup");
        return;
    }
    
    char *token = strtok(copia_buffer, " \t\n");
    if (token != NULL) {
        strcpy(primeira, token);
    } else {
        primeira[0] = '\0';
    }

    free(copia_buffer);
}

/**
 * @brief Obtem a segunda e a terceira palavra de um buffer de caracteres e escreve em outros buffers
 * 
 * @param buffer buffer de caracteres original
 * @param segunda  buffer 1 a ser escrito
 * @param terceira buffer 2 a ser escrito 
 */
void segunda_terceira_palavra(const char *buffer, char *segunda, char *terceira) {
  
    char *copia_buffer = strdup(buffer);
    if (copia_buffer == NULL) {
        perror("strdup");
        return;
    }
    
    char *token = strtok(copia_buffer, " \t\n");
    int contador = 0;
    
    while (token != NULL) {
        contador++;
        if (contador == 2) {
            strcpy(segunda, token);
        }
        if (contador == 3) {
            strcpy(terceira, token);
            break;
        }
        token = strtok(NULL, " \t\n");
    }
    
    if (contador < 2) {
        segunda[0] = '\0'; // Caso não exista a segunda palavra
    }
    if (contador < 3) {
        terceira[0] = '\0'; // Caso não exista a terceira palavra
    }

    free(copia_buffer);
}

int main(int argc, char **argv) {
	if (argc < 4) {
		usage(argc, argv);
	}

	bool mensagemDummy = false; // Controle da comunicacao. Usada para sincronizar client e servidor

	/* ---------- Conexao com Servidor SE ---------- */
	bool close_SE = false;
	struct sockaddr_storage storage_SE;
	if (0 != addrparse(argv[1], argv[2], &storage_SE)) {
		usage(argc, argv);
	}

	int s_SE;
	s_SE = socket(storage_SE.ss_family, SOCK_STREAM, 0);
	if (s_SE == -1) {
		logexit("socket");
	}
	struct sockaddr *addr_SE = (struct sockaddr *)(&storage_SE);
	if (0 != connect(s_SE, addr_SE, sizeof(storage_SE))) {
		logexit("connect");
	}

	char addrstr_SE[BUFSZ];
	addrtostr(addr_SE, addrstr_SE, BUFSZ);


	char buf_SE[BUFSZ];
	// Recebe a mensagem do servidor com respectivo ID_CLIENT
    size_t count_SE = recv(s_SE, buf_SE, BUFSZ - 1, 0);
    if (count_SE < 0) {
        logexit("recv");
    } else if (count_SE == 0) {
        logexit("Connection closed by server");
    }
    puts(buf_SE);

	/* ---------- Conexao com Servidor SCII ----------*/
	bool close_SCII = false;
	struct sockaddr_storage storage_SCII;
	if (0 != addrparse(argv[1], argv[3], &storage_SCII)) {
		usage(argc, argv);
	}
	
	int s_SCII;
	s_SCII = socket(storage_SCII.ss_family, SOCK_STREAM, 0);
	if (s_SCII == -1) {
		logexit("socket");
	}
	struct sockaddr *addr_SCII = (struct sockaddr *)(&storage_SCII);
	if (0 != connect(s_SCII, addr_SCII, sizeof(storage_SCII))) {
		logexit("connect");
	}

	char addrstr_SCII[BUFSZ];
	addrtostr(addr_SCII, addrstr_SCII, BUFSZ);


	char buf_SCII[BUFSZ];
	// Recebe a mensagem do servidor com respectivo ID_CLIENT
    size_t count_SCII = recv(s_SCII, buf_SCII, BUFSZ - 1, 0);
    if (count_SCII < 0) {
        logexit("recv");
    } else if (count_SCII == 0) {
        logexit("Connection closed by server");
    }
    puts(buf_SCII);
	
    while (1) {
		if(!mensagemDummy) {
			// Input mensagem
			char buf[BUFSZ];
			memset(buf, 0, BUFSZ);
			printf("mensagem> ");
			fgets(buf, BUFSZ - 1, stdin);
			size_t msg_len = strlen(buf);

			// Enviando mensagem para servidor SE
			if (send(s_SE, buf, msg_len, 0) != msg_len) {
				logexit("send");
			}
			// Enviando mensagem para servidor SCII
			if (send(s_SCII, buf, msg_len, 0) != msg_len) {
				logexit("send");
			}
		} else {
			send(s_SE, "...", strlen("...")+1, 0);
			send(s_SCII, "...", strlen("...")+1, 0);
		}
		mensagemDummy = false;

		// Recebendo resposta servidor SE
        count_SE = recv(s_SE, buf_SE, BUFSZ - 1, 0);
        if (count_SE < 0) {
            logexit("recv");
        } else if (count_SE == 0) {
            printf("Connection closed by server\n");
            break;
        }

		if (strcmp(buf_SE, "Successful disconnect\n") == 0) {
			close_SE = true;
		} else if (strcmp(buf_SE, "estado atual: alta") == 0) {
			mensagemDummy = true;
			puts(buf_SE);
			send(s_SCII, "REQ_UP", strlen("REQ_UP"), 0);
		} else if (strcmp(buf_SE, "estado atual: moderada") == 0) {
			mensagemDummy = true;
			puts(buf_SE);
			send(s_SCII, "REQ_NONE", strlen("REQ_NONE"), 0);
		}else if (strcmp(buf_SE, "estado atual: baixa") == 0) {
			mensagemDummy = true;
			puts(buf_SE);
			send(s_SCII, "REQ_DOWN", strlen("REQ_DOWN"), 0);
		}else {
			puts(buf_SE);
		}


		// Recebendo resposta servidor SCII
        count_SCII = recv(s_SCII, buf_SCII, BUFSZ - 1, 0);
        if (count_SCII < 0) {
            logexit("recv");
        } else if (count_SCII == 0) {
            printf("Connection closed by server\n");
            break;
        }

		char action[50];
		primeira_palavra(buf_SCII, action);

		if (strcmp(action, "RES_UP") == 0) {
			memset(action, 0, BUFSZ);
			char old_value[5];
			char new_value[5];
			segunda_terceira_palavra(buf_SCII, old_value, new_value);
			printf("consumo antigo: %s\nconsumo atual: %s\n", old_value, new_value);

		} else if(strcmp(action, "RES_NONE") == 0){
		    memset(action, 0, BUFSZ);
			char old_value[5];
			char dummy[5];
			segunda_terceira_palavra(buf_SCII, old_value, dummy);
			printf("consumo antigo: %s\n", old_value);

		}else if(strcmp(action, "RES_DOWN") == 0) {
			memset(action, 0, BUFSZ);
			char old_value[5];
			char new_value[5];
			segunda_terceira_palavra(buf_SCII, old_value, new_value);
			printf("consumo antigo: %s\nconsumo atual: %s\n", old_value, new_value);

		}  else if (strcmp(buf_SCII, "Successful disconnect\n") == 0) {
			close_SCII = true;
		} else {
			puts(buf_SCII);
		}

		if (close_SE || close_SCII) { break; }
    }

    close(s_SE);
	close(s_SCII);

    exit(EXIT_SUCCESS);
}