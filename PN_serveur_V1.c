/* PN_serveur_V1.c */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "pendu_ascii.h"

#define PORT 5000
#define MAX_ERREURS 6

void send_to_both(int c1, int c2, const char *msg) {
    send(c1, msg, strlen(msg), 0);
    send(c2, msg, strlen(msg), 0);
}

int main() {
    int sock_ecoute, client1, client2;
    struct sockaddr_in addr;
    char buffer[256];

    // SOCKET
    sock_ecoute = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_ecoute < 0) { 
        perror("socket"); 
        exit(1); 
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sock_ecoute, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind"); 
        exit(1);
    }

    if (listen(sock_ecoute, 2) < 0) {
        perror("listen"); 
        exit(1);
    }

    printf("Serveur en attente du client 1...\n");
    client1 = accept(sock_ecoute, NULL, NULL);
    printf("Client 1 connecté.\n");

    printf("Serveur en attente du client 2...\n");
    client2 = accept(sock_ecoute, NULL, NULL);
    printf("Client 2 connecté.\n");

    // --- SAISIE DU MOT ---
    char mot[64];
    printf("Entrez le mot à faire deviner : ");
    scanf("%63s", mot);

    int longueur = strlen(mot);
    char trouve[64];
    for (int i = 0; i < longueur; i++) trouve[i] = '_';
    trouve[longueur] = '\0';

    int erreurs = 0;
    int joueur_actif = 1;

    char lettres_proposees[26];
    int nb_lettres_proposees = 0;

    send_to_both(client1, client2, "Début du jeu du pendu !\n");

    while (1) {

        int actif  = (joueur_actif == 1 ? client1 : client2);
        int passif = (joueur_actif == 1 ? client2 : client1);

        // Envoi état actuel
        char msg[1024];
        char dessin[400];
        get_pendu_ascii(erreurs, dessin, sizeof(dessin));

        char lettres_affichees[80] = "Lettres proposées : ";
        if (nb_lettres_proposees == 0) {
            strcat(lettres_affichees, "aucune");
        } else {
            for (int i = 0; i < nb_lettres_proposees; i++) {
                char temp[4];
                snprintf(temp, sizeof(temp), "%c ", lettres_proposees[i]);
                strcat(lettres_affichees, temp);
            }
        }
        strcat(lettres_affichees, "\n");
    
        snprintf(msg, sizeof(msg),
            "\n%s\nMot : %s\nErreurs : %d / %d\nJoueur %d à vous :\n",
            dessin, trouve, erreurs, MAX_ERREURS, joueur_actif);

        send(actif, msg, strlen(msg), 0);
        send(passif, "ATTENTE...\n", 11, 0);

        // Demander lettre
        send(actif, "LETTRE:\n", 8, 0);

        memset(buffer, 0, 256);
        int n = recv(actif, buffer, 255, 0);
        if (n <= 0) break;

        char lettre = tolower(buffer[0]);

        int deja_proposee = 0;
        for (int i = 0; i < nb_lettres_proposees; i++) {
            if (lettres_proposees[i] == lettre) {
                deja_proposee = 1;
                break;
            }
        }
        
        if (deja_proposee) {
            char msg_erreur[128];
            snprintf(msg_erreur, sizeof(msg_erreur), 
                "\nLa lettre '%c' a déjà été proposée ! Veuillez réessayer.\n", lettre);
            send(actif, msg_erreur, strlen(msg_erreur), 0);
            // Ne pas changer de joueur, redemander une lettre
            continue;
        }

        lettres_proposees[nb_lettres_proposees++] = lettre;

        int bonne = 0;

        for (int i = 0; i < longueur; i++) {
            if (tolower(mot[i]) == lettre) {
                trouve[i] = mot[i];
                bonne = 1;
            }
        }

        if (!bonne) erreurs++;

        // Victoire
        if (strcmp(trouve, mot) == 0) {
            // Message pour le gagnant
            char msg_gagnant[256];
            snprintf(msg_gagnant, sizeof(msg_gagnant), 
                "\nVICTOIRE ! Vous avez trouvé le mot : %s\n", mot);
            send(actif, msg_gagnant, strlen(msg_gagnant), 0);
            
            // Message pour le perdant
            char msg_perdant[256];
            snprintf(msg_perdant, sizeof(msg_perdant), 
                "\nDEFAITE ! Votre adversaire a trouvé le mot : %s\n", mot);
            send(passif, msg_perdant, strlen(msg_perdant), 0);
            
            break;
        }

        // Défaite
        if (erreurs >= MAX_ERREURS) {
            // Message pour les deux joueurs (s'ils ont tous les deux perdu)
            char msg_defaite[256];
            snprintf(msg_defaite, sizeof(msg_defaite), 
                "\nDEFAITE ! Vous avez perdu. Le mot était : %s\n", mot);
            send_to_both(client1, client2, msg_defaite);
            
            break;
        }

        joueur_actif = (joueur_actif == 1 ? 2 : 1);
    }

    send_to_both(client1, client2, "FINI!\n");

    close(client1);
    close(client2);
    close(sock_ecoute);
    return 0;
}