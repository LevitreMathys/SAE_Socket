#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "pendu_ascii.h"
#include "game_pendu.h"

#define PORT 5000
#define ERREURS_MAX 6

struct roleJeu
{
    int numRole; // 0 = maitre du jeu | 1 = joueur
};

int main()
{
    while (1) // Boucle pour permettre de rejouer
    {
        char buffer[255];
        int socketToServeur, socketToClient;
        struct sockaddr_in adresse;
        socklen_t tailleAdresse = sizeof(adresse);
        struct roleJeu role;
        struct sockaddr_in infoClient;

        // CONNEXION AU SERVEUR
        socketToServeur = socket(AF_INET, SOCK_STREAM, 0);
        if (socketToServeur < 0)
        {
            perror("socket");
            char choix[10];
            printf("\nImpossible de créer le socket.\n");
            printf("Voulez-vous réessayer ? (o/n) : ");
            fgets(choix, sizeof(choix), stdin);
            if (choix[0] != 'o' && choix[0] != 'O')
                break;
            continue;
        }

        memset(&adresse, 0, tailleAdresse);
        adresse.sin_family = AF_INET;
        adresse.sin_addr.s_addr = inet_addr("127.0.0.1");
        adresse.sin_port = htons(PORT);

        if (connect(socketToServeur, (struct sockaddr *)&adresse, tailleAdresse) < 0)
        {
            perror("connect");
            close(socketToServeur);

            char choix[10];
            printf("\nImpossible de se connecter au serveur.\n");
            printf("Voulez-vous réessayer ? (o/n) : ");
            fgets(choix, sizeof(choix), stdin);
            if (choix[0] != 'o' && choix[0] != 'O')
                break;
            continue;
        }
        printf("Connexion au serveur établie.\n");

        // RÉCUPÉRATION DU RÔLE (ATTRIBUÉ PAR LE SERVEUR)
        int n = recv(socketToServeur, &role, sizeof(role), 0);
        if (n == -1)
        {
            perror("role");
            close(socketToServeur);
            continue;
        }
        printf("Rôle attribué par le serveur.\n");

        // RÉCUPÉRATION INFO CLIENT
        int i = recv(socketToServeur, &infoClient, sizeof(infoClient), 0);
        if (i == -1)
        {
            perror("info client");
            close(socketToServeur);
            continue;
        }
        printf("Récupération des infos client.\n");

        close(socketToServeur);
        printf("Déconnexion du serveur...\n\n");

        // ========================================
        // CONNEXION PEER-TO-PEER
        // ========================================

        if (role.numRole == 0)
        {
            // ===================================
            // MAITRE DU JEU : DEVIENT SERVEUR P2P
            // ===================================
            draw_logo();
            printf("Vous êtes le Maître du jeu (Client 1).\n");
            printf("Création d'un socket serveur pour attendre le joueur...\n");

            int socketServeurP2P = socket(AF_INET, SOCK_STREAM, 0);
            if (socketServeurP2P < 0)
            {
                perror("socket serveur P2P");
                continue;
            }

            int opt = 1;
            setsockopt(socketServeurP2P, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

            struct sockaddr_in adresseP2P;
            memset(&adresseP2P, 0, sizeof(adresseP2P));
            adresseP2P.sin_family = AF_INET;
            adresseP2P.sin_addr.s_addr = INADDR_ANY;
            adresseP2P.sin_port = htons(5001);

            if (bind(socketServeurP2P, (struct sockaddr *)&adresseP2P, sizeof(adresseP2P)) < 0)
            {
                perror("bind P2P");
                close(socketServeurP2P);
                continue;
            }

            if (listen(socketServeurP2P, 1) < 0)
            {
                perror("listen P2P");
                close(socketServeurP2P);
                continue;
            }

            printf("En attente de la connexion du joueur...\n");

            struct sockaddr_in adresseJoueur;
            socklen_t tailleJoueur = sizeof(adresseJoueur);
            socketToClient = accept(socketServeurP2P, (struct sockaddr *)&adresseJoueur, &tailleJoueur);

            if (socketToClient < 0)
            {
                perror("accept joueur");
                close(socketServeurP2P);
                continue;
            }

            printf("Joueur connecté ! Début de la partie.\n\n");
            close(socketServeurP2P);

            // ===================================
            // LOGIQUE DU MAITRE DU JEU
            // ===================================
            char mon_mot_secret[255] = "";
            char mot_courant[255] = "";
            char lettres_utilisees[255] = "";
            int nb_fautes = 0;
            int nb_lettres = 0;
            int partie_finie = 0;

            printf("Pensez à un mot et indiquez son nombre de lettres.\n");
            printf("\nQuel est votre mot secret ? : ");
            fgets(mon_mot_secret, sizeof(mon_mot_secret), stdin);
            mon_mot_secret[strcspn(mon_mot_secret, "\n")] = 0;

            // Convertir en majuscules
            for (int j = 0; mon_mot_secret[j]; j++)
            {
                mon_mot_secret[j] = toupper(mon_mot_secret[j]);
            }

            nb_lettres = strlen(mon_mot_secret);
            printf("Votre mot contient %d lettres.\n", nb_lettres);

            // Envoyer le nombre de lettres au joueur
            snprintf(buffer, sizeof(buffer), "%d", nb_lettres);
            if (send(socketToClient, buffer, strlen(buffer) + 1, 0) < 0)
            {
                perror("send nb_lettres");
                close(socketToClient);
                continue;
            }

            // Boucle de jeu
            while (!partie_finie)
            {
                int lus = recv(socketToClient, buffer, sizeof(buffer) - 1, 0);
                if (lus <= 0)
                {
                    printf("Le joueur a déconnecté.\n");
                    break;
                }
                buffer[lus] = '\0';

                // Vérifier si le joueur quitte
                if (strcmp(buffer, ".") == 0)
                {
                    printf("Le joueur a quitté la partie.\n");
                    partie_finie = 1;
                    break;
                }

                // Parser les messages du joueur
                if (buffer[0] == 'P') // Demande d'état
                {
                    char temp_lettres[255] = "";
                    int parsed = sscanf(buffer, "P:%d:%[^:]:%[^\n]", &nb_fautes, mot_courant, temp_lettres);

                    if (parsed >= 3)
                    {
                        strcpy(lettres_utilisees, temp_lettres);
                    }

                    printf("\n--- État actuel ---\n");
                    printf("Mot vu par le devineur : %s\n", mot_courant);
                    printf("Nombre de fautes : %d/6\n", nb_fautes);

                    if (strlen(lettres_utilisees) > 0)
                    {
                        printf("Lettres déjà utilisées : %s\n", lettres_utilisees);
                    }
                    printf("-------------------\n");
                }
                else if (buffer[0] == 'L') // Lettre proposée
                {
                    char lettre_proposee = buffer[2];

                    printf("\n=================================\n");
                    printf("Lettre proposée par le devineur : %c\n", lettre_proposee);
                    printf("=================================\n");
                    printf("Votre mot secret : %s\n", mon_mot_secret);

                    char reponse[10];
                    do
                    {
                        printf("Cette lettre est-elle dans VOTRE mot ? (o/n) : ");
                        fgets(reponse, sizeof(reponse), stdin);
                        reponse[strcspn(reponse, "\n")] = 0;
                    } while (strcmp(reponse, "o") != 0 && strcmp(reponse, "n") != 0);

                    if (reponse[0] == 'o')
                    {
                        char positions[255] = "";

                        printf("À quelles positions apparaît cette lettre ?\n");
                        printf("(numéros séparés par des espaces, ex: 1 3 5)\n");
                        printf("IMPORTANT : Le mot fait %d lettres, donnez des positions entre 1 et %d\n",
                               nb_lettres, nb_lettres);
                        printf("Positions : ");
                        fgets(positions, sizeof(positions), stdin);
                        positions[strcspn(positions, "\n")] = 0;

                        // Vérifier les positions
                        char positions_valides[255] = "";
                        char *token = strtok(positions, " ");
                        while (token != NULL)
                        {
                            int pos = atoi(token);
                            if (pos > 0 && pos <= nb_lettres)
                            {
                                if (strlen(positions_valides) > 0)
                                {
                                    strcat(positions_valides, " ");
                                }
                                strcat(positions_valides, token);
                            }
                            else
                            {
                                printf("Position %d ignorée (le mot fait %d lettres)\n", pos, nb_lettres);
                            }
                            token = strtok(NULL, " ");
                        }

                        // Envoyer positions + mot secret
                        char message_complet[512];
                        snprintf(message_complet, sizeof(message_complet), "%s MOT:%s",
                                 positions_valides, mon_mot_secret);
                        send(socketToClient, message_complet, strlen(message_complet) + 1, 0);
                    }
                    else
                    {
                        // Envoyer réponse vide + mot secret
                        char message_vide[255];
                        snprintf(message_vide, sizeof(message_vide), " MOT:%s", mon_mot_secret);
                        send(socketToClient, message_vide, strlen(message_vide) + 1, 0);
                    }
                }
                else if (buffer[0] == 'V') // Victoire du joueur
                {
                    char mot_final[255];
                    sscanf(buffer, "V:%[^\n]", mot_final);

                    printf("\n=================================\n");
                    printf("DÉFAITE ! Le devineur a trouvé votre mot.\n");
                    printf("Le mot était : %s\n", mot_final);
                    printf("=================================\n");
                    partie_finie = 1;
                }
                else if (buffer[0] == 'D') // Défaite du joueur
                {
                    char mot_final[255];
                    sscanf(buffer, "D:%[^\n]", mot_final);

                    printf("\n=================================\n");
                    printf("VICTOIRE ! Le devineur n'a pas trouvé votre mot.\n");
                    printf("Le mot était : %s\n", mot_final);
                    printf("=================================\n");
                    partie_finie = 1;
                }
            }

            close(socketToClient);
        }
        else if (role.numRole == 1)
        {
            // ===================================
            // JOUEUR : SE CONNECTE AU MAITRE P2P
            // ===================================
            draw_logo();
            printf("Vous êtes le Devineur (Client 2).\n");
            printf("Connexion au Maître du jeu...\n");

            // Petite attente pour laisser le temps au maître de créer son socket
            sleep(2);

            socketToClient = socket(AF_INET, SOCK_STREAM, 0);
            if (socketToClient < 0)
            {
                perror("socket client");
                continue;
            }

            // Se connecter au serveur P2P du maître (port 5001)
            infoClient.sin_port = htons(5001);

            if (connect(socketToClient, (struct sockaddr *)&infoClient, sizeof(infoClient)) < 0)
            {
                perror("connect au maitre");
                close(socketToClient);
                continue;
            }

            printf("Connexion au Maître du jeu établie !\n");
            printf("En attente du début de la partie...\n\n");

            // ===================================
            // LOGIQUE DU JOUEUR/DEVINEUR
            // ===================================
            int nb_fautes = 0;
            int partie_finie = 0;
            char mot_courant[255] = "";
            char lettres_utilisees[255] = "";
            int nb_lettres = 0;

            // Recevoir le nombre de lettres
            int lus = recv(socketToClient, buffer, sizeof(buffer) - 1, 0);
            if (lus <= 0)
            {
                printf("Erreur lors de la réception du nombre de lettres.\n");
                close(socketToClient);
                continue;
            }
            buffer[lus] = '\0';
            nb_lettres = atoi(buffer);

            // Initialiser le mot caché avec espaces
            for (int j = 0; j < nb_lettres; j++)
            {
                mot_courant[j * 2] = '_';
                if (j < nb_lettres - 1)
                {
                    mot_courant[j * 2 + 1] = ' ';
                }
            }
            mot_courant[nb_lettres * 2 - 1] = '\0';

            printf("Le mot à deviner contient %d lettres.\n", nb_lettres);
            printf("Bonne chance !\n\n");

            // Boucle de jeu
            while (!partie_finie)
            {
                // Afficher l'état actuel
                draw_pendu(nb_fautes, strlen(mot_courant));
                printf("Mot à deviner : %s\n", mot_courant);
                printf("Nombre de fautes : %d/%d\n", nb_fautes, ERREURS_MAX);

                if (strlen(lettres_utilisees) > 0)
                {
                    printf("Lettres déjà utilisées : %s\n", lettres_utilisees);
                }
                printf("\n");

                // Vérifier si le mot est complet (pas de '_')
                int complet = 1;
                for (int j = 0; mot_courant[j]; j++)
                {
                    if (mot_courant[j] == '_')
                    {
                        complet = 0;
                        break;
                    }
                }

                if (complet)
                {
                    // Victoire
                    snprintf(buffer, sizeof(buffer), "V:%s", mot_courant);
                    send(socketToClient, buffer, strlen(buffer) + 1, 0);

                    printf("\n=================================\n");
                    printf("VICTOIRE ! Vous avez trouvé le mot !\n");
                    printf("Le mot était : %s\n", mot_courant);
                    printf("=================================\n");
                    partie_finie = 1;
                    break;
                }

                if (nb_fautes >= ERREURS_MAX)
                {
                    // Défaite
                    snprintf(buffer, sizeof(buffer), "D:%s", mot_courant);
                    send(socketToClient, buffer, strlen(buffer) + 1, 0);

                    draw_pendu(ERREURS_MAX, strlen(mot_courant));
                    printf("\n=================================\n");
                    printf("DÉFAITE ! Vous avez été pendu !\n");
                    printf("=================================\n");
                    partie_finie = 1;
                    break;
                }

                // Demander une lettre
                char lettre_valide = 0;

                while (!lettre_valide)
                {
                    printf("Entrer une lettre ('.' pour quitter) : ");
                    fgets(buffer, sizeof(buffer), stdin);
                    buffer[strcspn(buffer, "\n")] = 0;

                    if (strcmp(buffer, ".") == 0)
                    {
                        send(socketToClient, buffer, strlen(buffer) + 1, 0);
                        partie_finie = 1;
                        break;
                    }

                    // Vérifier que c'est une lettre
                    if (strlen(buffer) == 1 && isalpha(buffer[0]))
                    {
                        char lettre = toupper(buffer[0]);

                        // Vérifier si déjà utilisée
                        if (strchr(lettres_utilisees, lettre) != NULL)
                        {
                            printf("Vous avez déjà utilisé la lettre '%c' !\n\n", lettre);
                            continue;
                        }

                        lettre_valide = 1;

                        // Envoyer la lettre au maître
                        snprintf(buffer, sizeof(buffer), "L:%c", lettre);
                        send(socketToClient, buffer, strlen(buffer) + 1, 0);

                        // Recevoir la réponse
                        lus = recv(socketToClient, buffer, sizeof(buffer) - 1, 0);
                        if (lus <= 0)
                        {
                            printf("Le Maître du jeu a fermé la connexion.\n");
                            partie_finie = 1;
                            break;
                        }
                        buffer[lus] = '\0';

                        // Parser la réponse (positions + mot)
                        char positions[255] = "";
                        char mot_secret[255] = "";

                        char *mot_ptr = strstr(buffer, "MOT:");
                        if (mot_ptr)
                        {
                            strcpy(mot_secret, mot_ptr + 4);
                            *mot_ptr = '\0'; // Couper pour récupérer les positions
                            strcpy(positions, buffer);
                        }

                        // Ajouter la lettre aux lettres utilisées
                        if (strlen(lettres_utilisees) > 0)
                        {
                            strcat(lettres_utilisees, " ");
                        }
                        char temp[2] = {lettre, '\0'};
                        strcat(lettres_utilisees, temp);

                        // Traiter les positions
                        if (strlen(positions) > 0 && positions[0] != ' ')
                        {
                            // Lettre trouvée
                            printf("\nBonne lettre ! Elle apparaît aux positions : %s\n\n", positions);

                            // Mettre à jour le mot caché (en tenant compte des espaces)
                            char *token = strtok(positions, " ");
                            while (token != NULL)
                            {
                                int pos = atoi(token);
                                if (pos > 0 && pos <= nb_lettres)
                                {
                                    mot_courant[(pos - 1) * 2] = lettre;
                                }
                                token = strtok(NULL, " ");
                            }
                        }
                        else
                        {
                            // Mauvaise lettre
                            printf("\nMauvaise lettre ! La lettre '%c' n'est pas dans le mot.\n\n", lettre);
                            nb_fautes++;
                        }

                        // Envoyer l'état actuel au maître
                        snprintf(buffer, sizeof(buffer), "P:%d:%s:%s",
                                 nb_fautes, mot_courant, lettres_utilisees);
                        send(socketToClient, buffer, strlen(buffer) + 1, 0);
                    }
                    else if (strlen(buffer) == 1 && isdigit(buffer[0]))
                    {
                        printf("'%s' n'est pas une lettre ! Entrez une lettre (a-z).\n\n", buffer);
                    }
                    else if (strlen(buffer) > 1)
                    {
                        printf("Veuillez entrer UNE SEULE lettre.\n\n");
                    }
                    else
                    {
                        printf("Entrée invalide. Veuillez entrer une lettre.\n\n");
                    }
                }
            }

            close(socketToClient);
        }

        // Demander si on veut rejouer
        char choix[10];
        printf("\nVoulez-vous rejouer ? (o/n) : ");
        fgets(choix, sizeof(choix), stdin);
        if (choix[0] != 'o' && choix[0] != 'O')
            break;
    }

    printf("Merci d'avoir joué !\n");
    return 0;
}