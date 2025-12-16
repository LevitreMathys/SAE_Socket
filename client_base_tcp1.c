#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
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
        printf("Cette lettre est-elle dans le mot ? (o/n) : ");
        fgets(buffer, sizeof(buffer), stdin);
        buffer[strcspn(buffer, "\n")] = 0;
        if (buffer[0] != 'o' && buffer[0] != 'n')
        {
            printf("Réponse invalide. Veuillez entrer 'o' pour oui ou 'n' pour non.\n");
            continue;
        }
        else
        {
            if (buffer[0] == 'o')
            {
                printf("Combien de fois cette lettre apparaît-elle dans le mot ? ");
                fgets(buffer, sizeof(buffer), stdin);
                buffer[strcspn(buffer, "\n")] = 0;
            }
            else
            {
                printf("Lettre refusée.\n");
            }
        }

    } while (buffer[0] != '.');

    close(socketClient);
    return 0;
}