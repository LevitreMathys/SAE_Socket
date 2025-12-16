#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "pendu_ascii.h"

#define PORT 5000

int main()
{

    char buffer[255];

    int socketClient;
    struct sockaddr_in adresse;
    socklen_t tailleAdresse = sizeof(adresse);

    socketClient = socket(AF_INET, SOCK_STREAM, 0);

    if (socketClient < 0)
    {
        perror("socket");
        exit(-4);
    }

    memset(&adresse, 0, tailleAdresse);
    adresse.sin_family = AF_INET;
    adresse.sin_addr.s_addr = inet_addr("127.0.0.1");
    adresse.sin_port = htons(PORT);

    if (connect(socketClient, (struct sockaddr *)&adresse, tailleAdresse) < 0)
    {
        perror("connect");
        exit(-6);
    }

    draw_logo();

    do
    {
        int n = recv(socketClient, buffer, sizeof(buffer) - 1, 0);

        if (n > 0)
        {
            buffer[n] = '\0';
            printf("Etat du mot : %s\n", buffer);
        }

        printf("Entrer une lettre ('.' pour quitter) : ");
        fgets(buffer, sizeof(buffer), stdin);

        buffer[strcspn(buffer, "\n")] = 0;
        if (strcmp(buffer, ".") == 0)
        {
            printf("Rupture de la connexion\n");
            break;
        }

        if (send(socketClient, buffer, strlen(buffer) + 1, 0) < 0)
        {
            perror("send");
            exit(-8);
        }

    } while (buffer[0] != '.');

    close(socketClient);
    return 0;
}