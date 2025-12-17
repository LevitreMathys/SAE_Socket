//client_base_tcp1.c - VERSION AMÉLIORÉE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <ctype.h>
#include "pendu_ascii.h"
#include "game_pendu.h"

#define PORT 5000

int main()
{
    while (1)
    {
        char buffer[255];
        int partie_finie = 0;
        int nb_fautes = 0;
        char mot_courant[255] = "";
        char lettres_utilisees[255] = "";  // NOUVEAU : stocker les lettres utilisées
        char mon_mot_secret[255] = "";     // NOUVEAU : le mot que j'ai choisi
        int nb_lettres = 0;                // NOUVEAU : nombre de lettres du mot

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
            close(socketClient);
            
            char choix[10];
            printf("\nImpossible de se connecter au serveur.\n");
            printf("Voulez-vous réessayer ? (o/n) : ");
            fgets(choix, sizeof(choix), stdin);
            if (choix[0] != 'o' && choix[0] != 'O') break;
            continue;
        }

        draw_logo();

        printf("Connexion au serveur établie.\n");
        printf("Vous êtes le Maître du jeu (Client 1).\n");
        printf("Pensez à un mot et indiquez son nombre de lettres.\n");
        
        // NOUVEAU : Demander aussi le mot secret
        printf("\nQuel est votre mot secret ? : ");
        fgets(mon_mot_secret, sizeof(mon_mot_secret), stdin);
        mon_mot_secret[strcspn(mon_mot_secret, "\n")] = 0;
        
        // Convertir en majuscules
        for (int i = 0; mon_mot_secret[i]; i++) {
            mon_mot_secret[i] = toupper(mon_mot_secret[i]);
        }
        
        nb_lettres = strlen(mon_mot_secret);
        printf("Votre mot contient %d lettres.\n", nb_lettres);
        
        // Envoyer le nombre de lettres
        snprintf(buffer, sizeof(buffer), "%d", nb_lettres);
        if (send(socketClient, buffer, strlen(buffer) + 1, 0) < 0)
        {
            perror("send");
            close(socketClient);
            continue;
        }

        do
        {
            int n = recv(socketClient, buffer, sizeof(buffer) - 1, 0);
            if (n <= 0)
                break;

            buffer[n] = '\0';

            // MODIFIÉ : Parser avec lettres utilisées (Format: P:nb_fautes:mot_cache:lettres)
            if (buffer[0] == 'P')
            {
                char temp_lettres[255] = "";
                int parsed = sscanf(buffer, "P:%d:%[^:]:%[^\n]", &nb_fautes, mot_courant, temp_lettres);
                
                if (parsed >= 3) {
                    strcpy(lettres_utilisees, temp_lettres);
                }
                
                printf("\n--- État actuel ---\n");
                printf("Mot vu par le devineur : %s\n", mot_courant);
                printf("Nombre de fautes : %d/6\n", nb_fautes);
                
                // NOUVEAU : Afficher lettres utilisées
                if (strlen(lettres_utilisees) > 0) {
                    printf("Lettres déjà utilisées : %s\n", lettres_utilisees);
                }
                
                printf("-------------------\n");
            }
            else if (buffer[0] == 'L')
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
                    // MODIFIÉ : Validation des positions
                    char positions[255] = "";
                    int position_count = 0;
                    
                    printf("À quelles positions apparaît cette lettre ?\n");
                    printf("(numéros séparés par des espaces, ex: 1 3 5)\n");
                    printf("IMPORTANT : Le mot fait %d lettres, donnez des positions entre 1 et %d\n", 
                           nb_lettres, nb_lettres);
                    printf("Positions : ");
                    fgets(positions, sizeof(positions), stdin);
                    positions[strcspn(positions, "\n")] = 0;
                    
                    // NOUVEAU : Vérifier les positions
                    char positions_valides[255] = "";
                    char *token = strtok(positions, " ");
                    while (token != NULL) {
                        int pos = atoi(token);
                        if (pos > 0 && pos <= nb_lettres) {
                            if (strlen(positions_valides) > 0) {
                                strcat(positions_valides, " ");
                            }
                            strcat(positions_valides, token);
                            position_count++;
                        } else {
                            printf("Position %d ignorée (le mot fait %d lettres)\n", pos, nb_lettres);
                        }
                        token = strtok(NULL, " ");
                    }
                    
                    // MODIFIÉ : Envoyer positions + mot secret
                    char message_complet[512];
                    snprintf(message_complet, sizeof(message_complet), "%s MOT:%s", 
                             positions_valides, mon_mot_secret);
                    send(socketClient, message_complet, strlen(message_complet) + 1, 0);
                }
                else
                {
                    // MODIFIÉ : Envoyer quand même le mot secret
                    char message_vide[255];
                    snprintf(message_vide, sizeof(message_vide), " MOT:%s", mon_mot_secret);
                    send(socketClient, message_vide, strlen(message_vide) + 1, 0);
                }
            }
            else if (buffer[0] == 'V')
            {
                char mot_final[255];
                sscanf(buffer, "V:%[^\n]", mot_final);
                
                printf("\n=================================\n");
                printf("VICTOIRE ! Le devineur n'a pas trouvé votre mot.\n");
                printf("Le mot était : %s\n", mot_final);
                printf("=================================\n");
                partie_finie = 1;
            }
            else if (buffer[0] == 'D')
            {
                char mot_final[255];
                sscanf(buffer, "D:%[^\n]", mot_final);
                
                printf("\n=================================\n");
                printf("DÉFAITE ! Le devineur a trouvé votre mot.\n");
                printf("Le mot était : %s\n", mot_final);
                printf("=================================\n");
                partie_finie = 1;
            }

        } while (!partie_finie);

        close(socketClient);
        
        char choix[10];
        printf("\nVoulez-vous rejouer ? (o/n) : ");
        fgets(choix, sizeof(choix), stdin);
        if (choix[0] != 'o' && choix[0] != 'O') break;
    }
    
    printf("Merci d'avoir joué !\n");
    return 0;
}
