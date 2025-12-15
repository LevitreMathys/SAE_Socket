/* PN_client_V1.c */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "pendu_ascii.h"

#define LG_MESSAGE 512
#define PORT 5000

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage : %s <ip> <port>\n", argv[0]);
        exit(1);
    }

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) { 
        perror("socket"); 
        exit(1); 
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(argv[2]));
    inet_pton(AF_INET, argv[1], &addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect");
        exit(1);
    }

    printf("Connecté au serveur.\n");

    char buffer[512];

    while (1) {
        memset(buffer, 0, sizeof(buffer));

        int n = recv(sock, buffer, sizeof(buffer)-1, 0);
        if (n <= 0) {
            printf("Serveur déconnecté.\n");
            break;
        }

        printf("%s", buffer);

        if (strstr(buffer, "FIN_PARTIE"))
            break;

        if (strstr(buffer, "LETTRE:")) {
            char c;
            printf("Votre lettre : ");
            scanf(" %c", &c);

            char msg[2] = { c, '\0' };
            send(sock, msg, 1, 0);
        }
    }

    close(sock);
    return 0;
}