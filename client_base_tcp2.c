//client_base_tcp2.c - VERSION AMÉLIORÉE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <ctype.h>
#include "pendu_ascii.h"

#define PORT 5000
#define ERREURS_MAX 6

int main()
{
    while (1)
    {
        int socketClient;
        struct sockaddr_in adresse;
        socklen_t tailleAdresse = sizeof(adresse);
        char buffer[255];
        int nb_fautes = 0;
        int partie_finie = 0;
        char mot_courant[255] = "";
        char lettres_utilisees[255] = "";  // NOUVEAU : stocker les lettres utilisées

        socketClient = socket(AF_INET, SOCK_STREAM, 0);
        if (socketClient < 0) { perror("socket"); exit(-4); }

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
        printf("Vous êtes le Devineur (Client 2).\n");
        printf("En attente du début de la partie...\n\n");

        while (!partie_finie)
        {
            int n = recv(socketClient, buffer, sizeof(buffer)-1, 0);
            if (n <= 0) break;
            buffer[n] = '\0';

            // MODIFIÉ : Parser avec lettres utilisées (Format: P:nb_fautes:mot_cache:lettres)
            if (buffer[0] == 'P')
            {
                char temp_lettres[255] = "";
                int parsed = sscanf(buffer, "P:%d:%[^:]:%[^\n]", &nb_fautes, mot_courant, temp_lettres);
                
                if (parsed >= 3) {
                    strcpy(lettres_utilisees, temp_lettres);
                }
                
                draw_pendu(nb_fautes, strlen(mot_courant));
                printf("Mot à deviner : %s\n", mot_courant);
                printf("Nombre de fautes : %d/%d\n", nb_fautes, ERREURS_MAX);
                
                // NOUVEAU : Afficher lettres utilisées
                if (strlen(lettres_utilisees) > 0) {
                    printf("Lettres déjà utilisées : %s\n", lettres_utilisees);
                }
                printf("\n");
                
                // Vérifier si le mot est complet
                int complet = 1;
                for (int i = 0; mot_courant[i]; i++) {
                    if (mot_courant[i] == '_') {
                        complet = 0;
                        break;
                    }
                }
                if (complet) {
                    partie_finie = 1;
                    continue;
                }
            }
            // NOUVEAU : Gérer les messages d'erreur
            else if (buffer[0] == 'E')
            {
                char message_erreur[255];
                sscanf(buffer, "E:%[^\n]", message_erreur);
                printf("\n ERREUR : %s\n\n", message_erreur);
                // Ne pas marquer comme partie_finie, continuer la boucle
            }
            else if (buffer[0] == 'V')
            { 
                char mot_final[255];
                sscanf(buffer, "V:%[^\n]", mot_final);
                
                printf("\n=================================\n");
                printf("VICTOIRE ! Vous avez trouvé le mot !\n");
                printf("Le mot était : %s\n", mot_final);
                printf("=================================\n");
                partie_finie = 1;
                continue;
            }
            else if (buffer[0] == 'D')
            { 
                char mot_final[255];
                sscanf(buffer, "D:%[^\n]", mot_final);
                
                draw_pendu(ERREURS_MAX, strlen(mot_final));
                printf("\n=================================\n");
                printf("DÉFAITE ! Vous avez été pendu !\n");
                printf("Le mot était : %s\n", mot_final);
                printf("=================================\n");
                partie_finie = 1;
                continue;
            }

            if (!partie_finie)
            {
                // MODIFIÉ : Validation de la saisie
                char lettre_valide = 0;
                
                while (!lettre_valide) {
                    printf("Entrer une lettre ('.' pour quitter) : ");
                    fgets(buffer, sizeof(buffer), stdin);
                    buffer[strcspn(buffer,"\n")] = 0;
                    
                    if (strcmp(buffer,".") == 0) {
                        send(socketClient, buffer, strlen(buffer)+1, 0);
                        partie_finie = 1;
                        break;
                    }
                    
                    // NOUVEAU : Vérifier que c'est une lettre
                    if (strlen(buffer) == 1 && isalpha(buffer[0])) {
                        lettre_valide = 1;
                        send(socketClient, buffer, strlen(buffer)+1, 0);
                    } else if (strlen(buffer) == 1 && isdigit(buffer[0])) {
                        printf("'%s' n'est pas une lettre ! Entrez une lettre (a-z).\n\n", buffer);
                    } else if (strlen(buffer) > 1) {
                        printf("Veuillez entrer UNE SEULE lettre.\n\n");
                    } else {
                        printf("Entrée invalide. Veuillez entrer une lettre.\n\n");
                    }
                }
            }
        }

        close(socketClient);

        char choix[10];
        printf("\nVoulez-vous rejouer ? (o/n) : ");
        fgets(choix, sizeof(choix), stdin);
        if (choix[0] != 'o' && choix[0] != 'O') break;
    }

    printf("Merci d'avoir joué !\n");
    return 0;
}
