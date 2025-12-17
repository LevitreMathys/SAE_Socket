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
    int partie_finie = 0;
    int nb_fautes = 0;
    char mot_courant[255] = "";

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
    printf("Vous êtes le Maître du jeu (Client 1).\n");
    printf("Pensez à un mot et indiquez son nombre de lettres.\n");
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
        if (n <= 0)
            break;

        buffer[n] = '\0';

        if (buffer[0] == 'P') // Format: P:nb_fautes:mot_cache
        {
            // Parser le message
            sscanf(buffer, "P:%d:%s", &nb_fautes, mot_courant);
            
            printf("\n--- État actuel ---\n");
            printf("Mot vu par le devineur : %s\n", mot_courant);
            printf("Nombre de fautes : %d/6\n", nb_fautes);
            printf("-------------------\n");
        }
        else if (buffer[0] == 'L')
        {
            printf("\n=================================\n");
            printf("Lettre proposée par le devineur : %c\n", buffer[2]);
            printf("=================================\n");
            
            char reponse[10];
            do
            {
                printf("Cette lettre est-elle dans VOTRE mot ? (o/n) : ");
                fgets(reponse, sizeof(reponse), stdin);
                reponse[strcspn(reponse, "\n")] = 0;
            } while (strcmp(reponse, "o") != 0 && strcmp(reponse, "n") != 0);

            if (reponse[0] == 'o')
            {
                char positions[255];
                printf("À quelles positions apparaît cette lettre ?\n");
                printf("(numéros séparés par des espaces, ex: 1 3 5) : ");
                fgets(positions, sizeof(positions), stdin);
                positions[strcspn(positions, "\n")] = 0;
                send(socketClient, positions, strlen(positions) + 1, 0);
            }
            else
            {
                char vide[] = "";
                send(socketClient, vide, 1, 0);
            }
        }
        else if (buffer[0] == 'V')
        {
            printf("\n=================================\n");
            printf("✅ VICTOIRE ! Le devineur n'a pas trouvé votre mot.\n");
            printf("=================================\n");
            partie_finie = 1;
        }
        else if (buffer[0] == 'D')
        {
            printf("\n=================================\n");
            printf("❌ DÉFAITE ! Le devineur a trouvé votre mot.\n");
            printf("=================================\n");
            partie_finie = 1;
        }

    } while (!partie_finie);

    close(socketClient);
    
    printf("\nPartie terminée. Merci d'avoir joué !\n");
    return 0;
}