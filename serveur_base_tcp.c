#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include "game_pendu.h"

#define PORT 5000

int main()
{
    Game game;
    int nb_lettres;

    int socketServeur, socketClient1, socketClient2;
    struct sockaddr_in adresseServeur, adresseClient;
    socklen_t tailleAdresseClient = sizeof(adresseClient);

    char *reponse;
    char message_recu[255], derniere_lettre;
    int characteres_lus;

    socketServeur = socket(AF_INET, SOCK_STREAM, 0);

    if (socketServeur < 0)
    {
        perror("socket");
        exit(-4);
    }
    printf("Socket serveur créée.\n");

    memset(&adresseServeur, 0, sizeof(adresseServeur));
    adresseServeur.sin_family = AF_INET;
    adresseServeur.sin_addr.s_addr = htonl(INADDR_ANY);
    adresseServeur.sin_port = htons(PORT);

    int opt = 1;
    setsockopt(socketServeur, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    if (bind(socketServeur, (struct sockaddr *)&adresseServeur, sizeof(adresseServeur)) < 0)
    {
        perror("bind");
        exit(-5);
    }

    if (listen(socketServeur, 3) < 0)
    {
        perror("listen");
        exit(-6);
    }

    printf("En attente de connexion de deux clients...\n");

    /* CONNEXION DU PREMIER CLIENT */
    socketClient1 = accept(socketServeur, (struct sockaddr *)&adresseClient, &tailleAdresseClient);
    printf("Client 1 connecté.\n");

    if (socketClient1 < 0)
    {
        perror("accept");
        exit(-7);
    }

    /* CONNEXION DU DEUXIEME CLIENT */
    socketClient2 = accept(socketServeur, (struct sockaddr *)&adresseClient, &tailleAdresseClient);
    printf("Client 2 connecté.\n");

    if (socketClient2 < 0)
    {
        perror("accept");
        exit(-7);
    }

    characteres_lus = recv(socketClient1, message_recu, sizeof(message_recu) - 1, 0);
    if (characteres_lus <= 0)
    {
        printf("La connexion a été coupée.\n");
        close(socketClient1);
        close(socketClient2);
        close(socketServeur);
        return 0;
    }
    message_recu[characteres_lus] = '\0';

    nb_lettres = atoi(message_recu);
    if (nb_lettres <= 0)
    {
        printf("La connexion a été coupée.\n");
        close(socketClient1);
        close(socketClient2);
        close(socketServeur);
        return 0;
    }
    init_word(&game, nb_lettres);
    reponse = getMotCache(&game);
    send(socketClient2, reponse, strlen(reponse) + 1, 0);

    fd_set fds;
    int maxfd = (socketClient1 > socketClient2 ? socketClient1 : socketClient2) + 1;

    while (1)
    {
        FD_ZERO(&fds);
        FD_SET(socketClient1, &fds);
        FD_SET(socketClient2, &fds);

        if (select(maxfd, &fds, NULL, NULL, NULL) < 0)
        {
            perror("select");
            break;
        }

        /* Client 2 a proposé une lettre */
        if (FD_ISSET(socketClient2, &fds))
        {
            int n = recv(socketClient2, message_recu, sizeof(message_recu) - 1, 0);
            if (n <= 0)
            {
                printf("Client 2 déconnecté\n");
                break;
            }
            message_recu[n] = '\0';
            derniere_lettre = message_recu[0];

            // Envoyer la lettre à Client 1 pour confirmation
            send(socketClient1, message_recu, n, 0);
        }

        /* Client 1 a confirmé */
        if (FD_ISSET(socketClient1, &fds))
        {
            int n = recv(socketClient1, message_recu, sizeof(message_recu) - 1, 0);
            if (n <= 0)
            {
                printf("Client 1 déconnecté\n");
                break;
            }
            message_recu[n] = '\0';

            // Mise à jour du mot caché
            char *token = strtok(message_recu, " ");
            while (token != NULL)
            {
                int pos = atoi(token);
                if (pos > 0 && pos <= nb_lettres)
                {
                    game.mot_cache[pos - 1] = derniere_lettre;
                }
                token = strtok(NULL, " ");
            }

            // Vérifier si toutes les lettres sont trouvées
            int victoire = 1;
            for (int i = 0; i < nb_lettres; i++)
            {
                if (game.mot_cache[i] == '_')
                {
                    victoire = 0;
                    break;
                }
            }

            // Envoyer l'état du mot mis à jour
            send(socketClient2, game.mot_cache, strlen(game.mot_cache) + 1, 0);
            send(socketClient1, game.mot_cache, strlen(game.mot_cache) + 1, 0);

            if (victoire)
            {
                char msg[] = "Victoire !";
                send(socketClient1, msg, strlen(msg) + 1, 0);
                send(socketClient2, msg, strlen(msg) + 1, 0);
                printf("Victoire ! Le mot était %s\n", game.mot_cache);
                break; // fin de la partie
            }
        }
    }

    close(socketClient1);
    close(socketClient2);
    close(socketServeur);

    return 0;
}