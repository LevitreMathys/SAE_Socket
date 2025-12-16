#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "pendu_ascii.h"
#include "game_pendu.h"

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

    printf("Connexion au serveur établie.\n");
    printf("Combien de lettres contient le mot à deviner ?\nNombre de lettres : ");
    fgets(buffer, sizeof(buffer), stdin);
    buffer[strcspn(buffer, "\n")] = 0;

    if (send(socketClient, buffer, strlen(buffer) + 1, 0) < 0)
    {
        perror("send");
        close(socketClient);
        return 0;
    }

    do
    {

        int n = recv(socketClient, buffer, sizeof(buffer) - 1, 0);
        if (n > 0)
        {
            buffer[n] = '\0';
            printf("Lettre proposée : %s\n", buffer);
        }
        do
        {
            printf("Cette lettre est-elle dans le mot ? (o/n) : ");
            fgets(buffer, sizeof(buffer), stdin);
            buffer[strcspn(buffer, "\n")] = 0;
        } while (strcmp(buffer, "o") != 0 && strcmp(buffer, "n") != 0);

        if (buffer[0] == 'o')
        {
            // Demander les positions
            char positions[255];
            printf("À quelles positions apparaît cette lettre ? (séparées par des espaces) : ");
            fgets(positions, sizeof(positions), stdin);
            positions[strcspn(positions, "\n")] = 0;

            // Envoyer les positions au serveur
            if (send(socketClient, positions, strlen(positions) + 1, 0) < 0)
            {
                perror("send");
                close(socketClient);
                return 0;
            }
        }
        else
        {
            // Si la lettre n'est pas dans le mot, envoyer une chaîne vide
            char vide[] = "";
            send(socketClient, vide, strlen(vide) + 1, 0);
        }

    } while (buffer[0] != '.');

    close(socketClient);
    return 0;
}
