//serveur_base_tcp.c - VERSION AMÉLIORÉE
#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <ctype.h>
#include "game_pendu.h"

#define PORT 5000
#define ERREURS_MAX 6

int main()
{
    int socketServeur;
    struct sockaddr_in adresseServeur;

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

    while (1)
    {
        Game game;
        int nb_lettres, nb_fautes = 0;
        int socketClient1, socketClient2;
        struct sockaddr_in adresseClient;
        socklen_t tailleAdresseClient = sizeof(adresseClient);
        char message_recu[255], derniere_lettre;
        int characteres_lus;
        char lettres_utilisees[255] = "";  // NOUVEAU : stocker les lettres utilisées
        char mot_secret[255] = "";          // NOUVEAU : stocker le mot secret

        printf("--- Nouvelle partie ---\n");
        printf("En attente de connexion de deux clients...\n");

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

        // MODIFIÉ : Envoyer état initial avec lettres utilisées
        char msg[512];
        snprintf(msg, sizeof(msg), "P:%d:%s:", nb_fautes, mot_espace);
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

            if (FD_ISSET(socketClient2, &fds))
            {
                int n = recv(socketClient2, message_recu, sizeof(message_recu)-1, 0);
                if (n <= 0) { printf("Client 2 déconnecté\n"); break; }
                message_recu[n] = '\0';
                
                if (strcmp(message_recu, ".") == 0) break;
                
                derniere_lettre = message_recu[0];

                // NOUVEAU : Vérifier que c'est une lettre
                if (!isalpha(derniere_lettre)) {
                    printf("Erreur : '%c' n'est pas une lettre. Ignoré.\n", derniere_lettre);
                    
                    // Renvoyer un message d'erreur au client 2
                    snprintf(msg, sizeof(msg), "E:Veuillez entrer une lettre uniquement");
                    send(socketClient2, msg, strlen(msg)+1, 0);
                    continue;
                }

                // NOUVEAU : Ajouter la lettre aux lettres utilisées
                derniere_lettre = tolower(derniere_lettre);
                int len = strlen(lettres_utilisees);
                if (len > 0) {
                    lettres_utilisees[len] = ' ';
                    lettres_utilisees[len+1] = derniere_lettre;
                    lettres_utilisees[len+2] = '\0';
                } else {
                    lettres_utilisees[0] = derniere_lettre;
                    lettres_utilisees[1] = '\0';
                }

                // Transmettre la lettre à Client1
                snprintf(msg, sizeof(msg), "L:%c", derniere_lettre);
                send(socketClient1, msg, strlen(msg)+1, 0);

                // Attendre la réponse de Client1
                n = recv(socketClient1, message_recu, sizeof(message_recu)-1, 0);
                if (n <= 0) { printf("Client 1 déconnecté\n"); break; }
                message_recu[n] = '\0';

                // NOUVEAU : Vérifier si le Client1 a envoyé le mot secret (format: "positions MOT:motSecret")
                char *mot_ptr = strstr(message_recu, "MOT:");
                if (mot_ptr != NULL) {
                    strcpy(mot_secret, mot_ptr + 4);  // Copier après "MOT:"
                    *mot_ptr = '\0';  // Terminer la chaîne avant "MOT:"
                    printf("Mot secret reçu : %s\n", mot_secret);
                }

                char *token = strtok(message_recu, " ");
                int correct = 0;
                while (token != NULL && strlen(token) > 0)
                {
                    int pos = atoi(token);
                    
                    // NOUVEAU : Valider que la position est <= nb_lettres
                    if (pos > 0 && pos <= nb_lettres)
                    {
                        game.mot_cache[pos-1] = derniere_lettre;
                        correct = 1;
                    }
                    else if (pos > nb_lettres)
                    {
                        printf("ALERTE : Client 1 a donné position %d mais le mot fait %d lettres !\n", 
                               pos, nb_lettres);
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

                // MODIFIÉ : Envoyer état avec lettres utilisées
                snprintf(msg, sizeof(msg), "P:%d:%s:%s", nb_fautes, mot_espace, lettres_utilisees);
                send(socketClient1, msg, strlen(msg)+1, 0);
                send(socketClient2, msg, strlen(msg)+1, 0);

                if (victoire)
                {
                    // MODIFIÉ : Utiliser le mot secret si disponible, sinon mot_espace
                    char *mot_a_afficher = (strlen(mot_secret) > 0) ? mot_secret : mot_espace;
                    
                    snprintf(msg, sizeof(msg), "V:%s", mot_a_afficher);
                    send(socketClient2, msg, strlen(msg)+1, 0);
                    snprintf(msg, sizeof(msg), "D:%s", mot_a_afficher);
                    send(socketClient1, msg, strlen(msg)+1, 0);
                    printf("Victoire du devineur ! Le mot était %s\n", mot_a_afficher);
                    partie_finie = 1;
                }
                else if (nb_fautes >= ERREURS_MAX)
                {
                    // MODIFIÉ : Utiliser le mot secret si disponible
                    char *mot_a_afficher = (strlen(mot_secret) > 0) ? mot_secret : mot_espace;
                    
                    snprintf(msg, sizeof(msg), "D:%s", mot_a_afficher);
                    send(socketClient2, msg, strlen(msg)+1, 0);
                    snprintf(msg, sizeof(msg), "V:%s", mot_a_afficher);
                    send(socketClient1, msg, strlen(msg)+1, 0);
                    printf("Défaite du devineur ! Le mot était %s\n", mot_a_afficher);
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