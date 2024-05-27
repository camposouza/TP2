#include "common.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

void usage(int argc, char **argv) {
	printf("usage: %s <server IP> <server port 1> <server port 2>\n", argv[0]);
	printf("example: %s 127.0.0.1 12345 54321\n", argv[0]);
	exit(EXIT_FAILURE);
}

#define BUFSZ 1024

int main(int argc, char **argv) {
	if (argc < 4) {
		usage(argc, argv);
	}

	/* ---------- Conexao com Servidor SE ---------- */
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

	printf("connected to %s\n", addrstr_SE);

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

	printf("connected to %s\n", addrstr_SCII);

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
		// Input mensagem
		char buf[BUFSZ];
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

		// Recebendo resposta servidor SE
        count_SE = recv(s_SE, buf_SE, BUFSZ - 1, 0);
        if (count_SE < 0) {
            logexit("recv");
        } else if (count_SE == 0) {
            printf("Connection closed by server\n");
            break;
        }
        puts(buf_SE);

		// Recebendo resposta servidor SCII
        count_SCII = recv(s_SCII, buf_SCII, BUFSZ - 1, 0);
        if (count_SCII < 0) {
            logexit("recv");
        } else if (count_SCII == 0) {
            printf("Connection closed by server\n");
            break;
        }
        puts(buf_SCII);
    }

    close(s_SE);
	close(s_SCII);

    exit(EXIT_SUCCESS);
}