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
    int sock;
    struct sockaddr_in serv;
    socklen_t len;

    char buffer[32];
    char reponse[512];
    int lus;

    char ip[16];
    int port;

    if (argc < 3)
    {
        printf("USAGE : %s ip port\n", argv[0]);
        exit(1);
    }

    strncpy(ip, argv[1], 15);
    ip[15] = '\0';
    port = atoi(argv[2]);

    // ------------------------------------------------------
    // Connexion serveur
    // ------------------------------------------------------
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        perror("socket");
        exit(1);
    }

    len = sizeof(serv);
    memset(&serv, 0, len);
    serv.sin_family = AF_INET;
    serv.sin_port = htons(port);
    inet_aton(ip, &serv.sin_addr);

    if (connect(sock, (struct sockaddr *)&serv, len) < 0)
    {
        perror("connect");
        close(sock);
        exit(2);
    }
    printf("Connexion au serveur %s:%d réussie !\n\n", ip, port);

    // ======================================================
    //   BOUCLE PRINCIPALE DU JEU
    // ======================================================
    int quitter = 0;

    while (!quitter)
    {

        do
        {
            printf("Tapez \"start x\" pour commencer ou \".\" pour quitter : ");
            fgets(buffer, sizeof(buffer), stdin);

            // enlever le \n en fin
            buffer[strcspn(buffer, "\n")] = 0;

            if (strcmp(buffer, ".") == 0)
            {
                quitter = 1;
                break;
            }

            if (strcmp(buffer, "start x") == 0)
            {
                break; // buffer contient maintenant "start x"
            }

            printf("Commande invalide.\n");

        } while (1);

        if (quitter)
            break;

        printf("\n--- Nouvelle partie ! ---\n");

        // --------------------------------------------------
        // Boucle d'une partie
        // --------------------------------------------------
        while (1)
        {
            printf("Entrer une lettre ('.' pour quitter) : ");
            scanf("%31s", buffer);

            if (strcmp(buffer, ".") == 0)
            {
                quitter = 1;
                break;
            }

            // envoyer la lettre
            send(sock, buffer, strlen(buffer) + 1, 0);

            // recevoir la réponse
            lus = recv(sock, reponse, sizeof(reponse) - 1, 0);
            if (lus <= 0)
            {
                printf("Le serveur a fermé la connexion.\n");
                quitter = 1;
                break;
            }

            reponse[lus] = '\0';
            printf("%s\n", reponse);

            // dessiner pendu
            int fautes = 0;
            char *ptr = strstr(reponse, "Fautes : ");
            if (ptr)
                sscanf(ptr, "Fautes : %d", &fautes);
            draw_pendu(fautes, 6);

            // Fin de partie ?
            if (strstr(reponse, "Félicitations") || strstr(reponse, "Perdu"))
            {
                printf("\nVoulez-vous rejouer ? (start x) ou quitter (.)\n");
                break; // revenir au menu principal
            }
        } // fin boucle partie

    } // fin boucle principale

    printf("Déconnexion...\n");
    close(sock);
    return 0;
}
