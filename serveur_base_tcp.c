#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include "game_pendu.h"

#define PORT 5000
#define ERREURS_MAX 6

int main()
{
    int socketServeur;
    struct sockaddr_in adresseServeur;

    // Création du serveur (une seule fois)
    socketServeur = socket(AF_INET, SOCK_STREAM, 0);
    if (socketServeur < 0) { perror("socket"); exit(-4); }

    memset(&adresseServeur, 0, sizeof(adresseServeur));
    adresseServeur.sin_family = AF_INET;
    adresseServeur.sin_addr.s_addr = htonl(INADDR_ANY);
    adresseServeur.sin_port = htons(PORT);

    int opt = 1;
    setsockopt(socketServeur, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    if (bind(socketServeur, (struct sockaddr *)&adresseServeur, sizeof(adresseServeur)) < 0) { perror("bind"); exit(-5); }
    if (listen(socketServeur, 3) < 0) { perror("listen"); exit(-6); }

    printf("=== Serveur de jeu du Pendu démarré ===\n");
    printf("Port : %d\n", PORT);
    printf("En attente de joueurs...\n\n");

    while (1)  // Le serveur reste en marche indéfiniment
    {
        Game game;
        int nb_lettres, nb_fautes = 0;
        int socketClient1, socketClient2;
        struct sockaddr_in adresseClient;
        socklen_t tailleAdresseClient = sizeof(adresseClient);
        char message_recu[255], derniere_lettre;
        int characteres_lus;

        printf("--- Nouvelle partie ---\n");
        printf("En attente de connexion de deux clients...\n");

        // Connexion des clients
        socketClient1 = accept(socketServeur, (struct sockaddr *)&adresseClient, &tailleAdresseClient);
        printf("Client 1 connecté (Maître du jeu).\n");
        socketClient2 = accept(socketServeur, (struct sockaddr *)&adresseClient, &tailleAdresseClient);
        printf("Client 2 connecté (Devineur).\n");
        printf("Les deux joueurs sont connectés ! Début de la partie...\n\n");

        // Recevoir nombre de lettres
        characteres_lus = recv(socketClient1, message_recu, sizeof(message_recu) - 1, 0);
        if (characteres_lus <= 0) { 
            close(socketClient1); 
            close(socketClient2); 
            continue; 
        }
        message_recu[characteres_lus] = '\0';
        nb_lettres = atoi(message_recu);
        init_word(&game, nb_lettres);

        // Ajouter espaces entre les underscores
        char mot_espace[512];
        int j = 0;
        for (int i = 0; i < nb_lettres; i++) {
            mot_espace[j++] = game.mot_cache[i];
            if (i < nb_lettres - 1) mot_espace[j++] = ' ';
        }
        mot_espace[j] = '\0';

        // Envoyer état initial aux deux clients
        char msg[512];
        snprintf(msg, sizeof(msg), "P:%d:%s", nb_fautes, mot_espace);
        send(socketClient1, msg, strlen(msg)+1, 0);
        send(socketClient2, msg, strlen(msg)+1, 0);

        fd_set fds;
        int maxfd = (socketClient1 > socketClient2 ? socketClient1 : socketClient2) + 1;
        int partie_finie = 0;

        while (!partie_finie)
        {
            FD_ZERO(&fds);
            FD_SET(socketClient2, &fds);

            if (select(maxfd, &fds, NULL, NULL, NULL) < 0) { perror("select"); break; }

            // Client 2 propose une lettre
            if (FD_ISSET(socketClient2, &fds))
            {
                int n = recv(socketClient2, message_recu, sizeof(message_recu)-1, 0);
                if (n <= 0) { printf("Client 2 déconnecté\n"); break; }
                message_recu[n] = '\0';
                
                if (strcmp(message_recu, ".") == 0) break;
                
                derniere_lettre = message_recu[0];

                // Transmettre la lettre à Client1
                snprintf(msg, sizeof(msg), "L:%c", derniere_lettre);
                send(socketClient1, msg, strlen(msg)+1, 0);

                // Attendre la réponse de Client1
                n = recv(socketClient1, message_recu, sizeof(message_recu)-1, 0);
                if (n <= 0) { printf("Client 1 déconnecté\n"); break; }
                message_recu[n] = '\0';

                char *token = strtok(message_recu, " ");
                int correct = 0;
                while (token != NULL && strlen(token) > 0)
                {
                    int pos = atoi(token);
                    if (pos > 0 && pos <= nb_lettres)
                    {
                        game.mot_cache[pos-1] = derniere_lettre;
                        correct = 1;
                    }
                    token = strtok(NULL, " ");
                }

                if (!correct) nb_fautes++;

                // Vérifier victoire
                int victoire = 1;
                for (int i = 0; i < nb_lettres; i++)
                    if (game.mot_cache[i] == '_') { victoire = 0; break; }

                // Reformater avec espaces
                j = 0;
                for (int i = 0; i < nb_lettres; i++) {
                    mot_espace[j++] = game.mot_cache[i];
                    if (i < nb_lettres - 1) mot_espace[j++] = ' ';
                }
                mot_espace[j] = '\0';

                // Envoyer état mis à jour aux deux clients
                snprintf(msg, sizeof(msg), "P:%d:%s", nb_fautes, mot_espace);
                send(socketClient1, msg, strlen(msg)+1, 0);
                send(socketClient2, msg, strlen(msg)+1, 0);

                if (victoire)
                {
                    // Envoyer le mot complet au client2
                    snprintf(msg, sizeof(msg), "V:%s", mot_espace);
                    send(socketClient2, msg, strlen(msg)+1, 0);
                    send(socketClient1, "D", 2, 0);
                    printf("Victoire du devineur ! Le mot était %s\n", game.mot_cache);
                    partie_finie = 1;
                }
                else if (nb_fautes >= ERREURS_MAX)
                {
                    // Envoyer le mot complet au client2
                    snprintf(msg, sizeof(msg), "D:%s", mot_espace);
                    send(socketClient2, msg, strlen(msg)+1, 0);
                    send(socketClient1, "V", 2, 0);
                    printf("Défaite du devineur ! Le mot était %s\n", game.mot_cache);
                    partie_finie = 1;
                }
            }
        }

        close(socketClient1);
        close(socketClient2);
        printf("Partie terminée. En attente de nouveaux joueurs...\n\n");
    }

    close(socketServeur);
    return 0;
}