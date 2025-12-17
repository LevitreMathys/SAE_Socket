#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <ctype.h>
#include <sys/wait.h>
#include "game_pendu.h"

#define PORT 5000
#define ERREURS_MAX 6

void gerer_partie(int socketClient1, int socketClient2) {
    Game game;
    int nb_lettres, nb_fautes = 0;
    char message_recu[255], derniere_lettre;
    int characteres_lus;
    char lettres_utilisees[255] = "";
    char mot_secret[255] = "";
    char msg[512];
    
    int mon_pid = getpid();  // Récupérer le PID du processus

    printf("[PARTIE %d] Démarrage d'une nouvelle partie\n", mon_pid);

    // ==========================================
    // INITIALISATION DE LA PARTIE
    // ==========================================
    
    // Recevoir nombre de lettres du maître du jeu (Client 1)
    characteres_lus = recv(socketClient1, message_recu, sizeof(message_recu) - 1, 0);
    if (characteres_lus <= 0) {
        printf("[PARTIE %d] Erreur: Client 1 déconnecté lors de l'initialisation\n", mon_pid);
        return;
    }
    message_recu[characteres_lus] = '\0';
    nb_lettres = atoi(message_recu);
    printf("[PARTIE %d] Nombre de lettres: %d\n", mon_pid, nb_lettres);
    
    init_word(&game, nb_lettres);

    // Ajouter espaces entre les underscores pour l'affichage
    char mot_espace[512];
    int j = 0;
    for (int i = 0; i < nb_lettres; i++) {
        mot_espace[j++] = game.mot_cache[i];
        if (i < nb_lettres - 1) mot_espace[j++] = ' ';
    }
    mot_espace[j] = '\0';

    // Envoyer état initial aux deux clients (P:nb_fautes:mot_cache:lettres_utilisees)
    snprintf(msg, sizeof(msg), "P:%d:%s:", nb_fautes, mot_espace);
    send(socketClient1, msg, strlen(msg) + 1, 0);
    send(socketClient2, msg, strlen(msg) + 1, 0);
    printf("[PARTIE %d] État initial envoyé: %s\n", mon_pid, msg);

    // ==========================================
    // BOUCLE PRINCIPALE DE JEU
    // ==========================================
    
    fd_set fds;
    int maxfd = (socketClient1 > socketClient2 ? socketClient1 : socketClient2) + 1;
    int partie_finie = 0;

    while (!partie_finie) {
        FD_ZERO(&fds);
        FD_SET(socketClient2, &fds); // On attend une lettre du joueur (Client 2)

        if (select(maxfd, &fds, NULL, NULL, NULL) < 0) {
            perror("[PARTIE] select");
            break;
        }

        if (FD_ISSET(socketClient2, &fds)) {
            // Recevoir la lettre proposée par le joueur
            int n = recv(socketClient2, message_recu, sizeof(message_recu) - 1, 0);
            if (n <= 0) {
                printf("[PARTIE %d] Client 2 (joueur) déconnecté\n", mon_pid);
                break;
            }
            message_recu[n] = '\0';

            // Commande d'arrêt
            if (strcmp(message_recu, ".") == 0) {
                printf("[PARTIE %d] Arrêt demandé\n", mon_pid);
                break;
            }

            derniere_lettre = message_recu[0];

            // Vérifier que c'est bien une lettre
            if (!isalpha(derniere_lettre)) {
                printf("[PARTIE %d] Caractère invalide reçu: '%c'\n", mon_pid, derniere_lettre);
                snprintf(msg, sizeof(msg), "E:Veuillez entrer une lettre uniquement");
                send(socketClient2, msg, strlen(msg) + 1, 0);
                continue;
            }

            derniere_lettre = tolower(derniere_lettre);
            printf("[PARTIE %d] Lettre proposée: %c\n", mon_pid, derniere_lettre);

            // Ajouter la lettre à la liste des lettres utilisées
            int len = strlen(lettres_utilisees);
            if (len > 0) {
                lettres_utilisees[len] = ' ';
                lettres_utilisees[len + 1] = derniere_lettre;
                lettres_utilisees[len + 2] = '\0';
            } else {
                lettres_utilisees[0] = derniere_lettre;
                lettres_utilisees[1] = '\0';
            }

            // Transmettre la lettre au maître du jeu (Client 1)
            snprintf(msg, sizeof(msg), "L:%c", derniere_lettre);
            send(socketClient1, msg, strlen(msg) + 1, 0);

            // Attendre la réponse du maître du jeu avec les positions
            n = recv(socketClient1, message_recu, sizeof(message_recu) - 1, 0);
            if (n <= 0) {
                printf("[PARTIE %d] Client 1 (maître) déconnecté\n", mon_pid);
                break;
            }
            message_recu[n] = '\0';

            // Vérifier si le Client 1 a envoyé le mot secret (format: "positions MOT:motSecret")
            char *mot_ptr = strstr(message_recu, "MOT:");
            if (mot_ptr != NULL) {
                strcpy(mot_secret, mot_ptr + 4);
                *mot_ptr = '\0'; // Terminer avant "MOT:"
                printf("[PARTIE %d] Mot secret reçu: %s\n", mon_pid, mot_secret);
            }

            // Traiter les positions
            char *token = strtok(message_recu, " ");
            int correct = 0;
            while (token != NULL && strlen(token) > 0) {
                int pos = atoi(token);
                if (pos > 0 && pos <= nb_lettres) {
                    game.mot_cache[pos - 1] = derniere_lettre;
                    correct = 1;
                    printf("[PARTIE %d] Lettre correcte à la position %d\n", mon_pid, pos);
                } else if (pos > nb_lettres) {
                    printf("[PARTIE %d] ALERTE: Position invalide %d (max=%d)\n", mon_pid, pos, nb_lettres);
                }
                token = strtok(NULL, " ");
            }

            if (!correct) {
                nb_fautes++;
                printf("[PARTIE %d] Lettre incorrecte. Fautes: %d/%d\n", mon_pid, nb_fautes, ERREURS_MAX);
            }

            // Vérifier victoire (tous les underscores remplacés)
            int victoire = 1;
            for (int i = 0; i < nb_lettres; i++) {
                if (game.mot_cache[i] == '_') {
                    victoire = 0;
                    break;
                }
            }

            // Reformater le mot avec espaces
            j = 0;
            for (int i = 0; i < nb_lettres; i++) {
                mot_espace[j++] = game.mot_cache[i];
                if (i < nb_lettres - 1) mot_espace[j++] = ' ';
            }
            mot_espace[j] = '\0';

            // Envoyer état actuel aux deux clients
            snprintf(msg, sizeof(msg), "P:%d:%s:%s", nb_fautes, mot_espace, lettres_utilisees);
            send(socketClient1, msg, strlen(msg) + 1, 0);
            send(socketClient2, msg, strlen(msg) + 1, 0);

            // ==========================================
            // VÉRIFICATION FIN DE PARTIE
            // ==========================================
            
            if (victoire) {
                // Le joueur a gagné
                char *mot_a_afficher = (strlen(mot_secret) > 0) ? mot_secret : mot_espace;
                
                snprintf(msg, sizeof(msg), "V:%s", mot_a_afficher);
                send(socketClient2, msg, strlen(msg) + 1, 0); // Victoire pour le joueur
                
                snprintf(msg, sizeof(msg), "D:%s", mot_a_afficher);
                send(socketClient1, msg, strlen(msg) + 1, 0); // Défaite pour le maître
                
                printf("[PARTIE %d] VICTOIRE du joueur! Mot: %s\n", mon_pid, mot_a_afficher);
                partie_finie = 1;
                
            } else if (nb_fautes >= ERREURS_MAX) {
                // Le maître du jeu a gagné
                char *mot_a_afficher = (strlen(mot_secret) > 0) ? mot_secret : mot_espace;
                
                snprintf(msg, sizeof(msg), "D:%s", mot_a_afficher);
                send(socketClient2, msg, strlen(msg) + 1, 0); // Défaite pour le joueur
                
                snprintf(msg, sizeof(msg), "V:%s", mot_a_afficher);
                send(socketClient1, msg, strlen(msg) + 1, 0); // Victoire pour le maître
                
                printf("[PARTIE %d] DÉFAITE du joueur! Mot: %s\n", mon_pid, mot_a_afficher);
                partie_finie = 1;
            }
        }
    }

    printf("[PARTIE %d] Partie terminée\n\n", mon_pid);
}

int main() {
    struct sockaddr_in adresseServeur, adresseClient;
    socklen_t tailleAdresseClient = sizeof(adresseClient);

    // ==========================================
    // CRÉATION DU SOCKET SERVEUR
    // ==========================================
    
    int socketServeur = socket(AF_INET, SOCK_STREAM, 0);
    if (socketServeur < 0) {
        perror("socket");
        exit(-4);
    }

    memset(&adresseServeur, 0, sizeof(adresseServeur));
    adresseServeur.sin_family = AF_INET;
    adresseServeur.sin_addr.s_addr = htonl(INADDR_ANY);
    adresseServeur.sin_port = htons(PORT);

    int opt = 1;
    setsockopt(socketServeur, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    if (bind(socketServeur, (struct sockaddr *)&adresseServeur, sizeof(adresseServeur)) < 0) {
        perror("bind");
        exit(-5);
    }

    if (listen(socketServeur, 10) < 0) {
        perror("listen");
        exit(-6);
    }

    printf("===========================================\n");
    printf("SERVEUR PENDU - VERSION 3 (Multi-parties)\n");
    printf("===========================================\n");
    printf("Port: %d\n", PORT);
    printf("PID du serveur: %d\n", getpid());
    printf("En attente de connexions...\n\n");

    // ==========================================
    // BOUCLE PRINCIPALE : ACCEPTER DES PARTIES
    // ==========================================
    
    while (1) {
        printf("[SERVEUR] En attente de 2 clients pour une nouvelle partie...\n");

        // Accepter le premier client (Maître du jeu)
        int socketClient1 = accept(socketServeur, (struct sockaddr *)&adresseClient, &tailleAdresseClient);
        if (socketClient1 < 0) {
            perror("accept client1");
            continue;
        }
        printf("[SERVEUR] Client 1 (Maître du jeu) connecté - Socket: %d\n", socketClient1);

        // Accepter le deuxième client (Joueur)
        int socketClient2 = accept(socketServeur, (struct sockaddr *)&adresseClient, &tailleAdresseClient);
        if (socketClient2 < 0) {
            perror("accept client2");
            close(socketClient1);
            continue;
        }
        printf("[SERVEUR] Client 2 (Joueur) connecté - Socket: %d\n", socketClient2);

        // ==========================================
        // FORK : CRÉER UN PROCESSUS FILS
        // ==========================================
        
        int pid = fork();

        if (pid < 0) {
            perror("fork");
            close(socketClient1);
            close(socketClient2);
            continue;
        }

        if (pid == 0) {
            // ==========================================
            // PROCESSUS FILS : GÈRE LA PARTIE
            // ==========================================
            
            close(socketServeur); // Le fils n'a pas besoin du socket serveur
            
            printf("[FILS %d] Gestion de la partie\n", getpid());
            
            gerer_partie(socketClient1, socketClient2);
            
            // Fermer les sockets clients
            close(socketClient1);
            close(socketClient2);
            
            printf("[FILS %d] Terminaison du processus\n", getpid());
            exit(0);
            
        } else {
            // ==========================================
            // PROCESSUS PÈRE : CONTINUE D'ACCEPTER
            // ==========================================
            
            printf("[SERVEUR] Partie lancée dans le processus fils (PID=%d)\n", pid);
            
            // Le père ferme ses copies des sockets clients
            close(socketClient1);
            close(socketClient2);
            
            // Nettoyer les processus fils zombies (non-bloquant)
            while (waitpid(-1, NULL, WNOHANG) > 0);
            
            printf("[SERVEUR] Prêt pour une nouvelle partie\n\n");
        }
    }

    close(socketServeur);
    return 0;
}