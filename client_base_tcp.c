#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "pendu_ascii.h"

int main(int argc, char *argv[])
{
    int descripteurSocket;
    struct sockaddr_in sockaddrDistant;
    socklen_t longueurAdresse;

    char buffer[10] = "";
    int nb;

    char ip_dest[16];
    int port_dest;

    char reponse[512]; // augmenter taille pour recevoir tout le message
    int lus;

    if (argc > 1)
    {
        strncpy(ip_dest, argv[1], 16);
        sscanf(argv[2], "%d", &port_dest);
    }
    else
    {
        printf("USAGE : %s ip port\n", argv[0]);
        exit(-1);
    }

    descripteurSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (descripteurSocket < 0)
    {
        perror("Erreur en création de la socket...");
        exit(-1);
    }
    printf("Socket créée! (%d)\n", descripteurSocket);

    longueurAdresse = sizeof(sockaddrDistant);
    memset(&sockaddrDistant, 0x00, longueurAdresse);
    sockaddrDistant.sin_family = AF_INET;
    sockaddrDistant.sin_port = htons(port_dest);
    inet_aton(ip_dest, &sockaddrDistant.sin_addr);

    if ((connect(descripteurSocket, (struct sockaddr *)&sockaddrDistant, longueurAdresse)) == -1)
    {
        perror("Erreur de connection avec le serveur distant...");
        close(descripteurSocket);
        exit(-2);
    }
    printf("Connexion au serveur %s:%d réussie!\n", ip_dest, port_dest);

    while (1)
    {
        printf("Entrer une lettre ('.' pour quitter) : ");
        scanf(" %c", &buffer[0]);
        buffer[1] = '\0';

        if (buffer[0] == '.')
        {
            printf("Déconnexion volontaire du client...\n");
            break;
        }

        nb = send(descripteurSocket, buffer, strlen(buffer) + 1, 0);
        if (nb <= 0)
        {
            perror("Erreur en écriture...");
            close(descripteurSocket);
            exit(-3);
        }

        lus = recv(descripteurSocket, reponse, sizeof(reponse) - 1, 0);
        if (lus > 0)
        {
            reponse[lus] = '\0';
            printf("%s\n", reponse);

            int nb_fautes = 0;
            char *ptr = strstr(reponse, "Fautes : ");
            if (ptr != NULL)
            {
                sscanf(ptr, "Fautes : %d", &nb_fautes);
            }

            draw_pendu(nb_fautes, 5);

            if (strstr(reponse, "Félicitations !") || strstr(reponse, "Perdu !"))
                break;
        }
        else if (lus == 0)
        {
            printf("Le serveur a fermé la connexion.\n");
            break;
        }
        else
        {
            perror("Erreur recv");
            close(descripteurSocket);
            exit(-4);
        }
    }

    printf("Fermeture du programme\n");
    close(descripteurSocket);

    return 0;
}
