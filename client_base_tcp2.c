#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "pendu_ascii.h"

#define PORT 5000
#define ERREURS_MAX 6

int main()
{
    while (1) // boucle pour rejouer
    {
        int socketClient;
        struct sockaddr_in adresse;
        socklen_t tailleAdresse = sizeof(adresse);
        char buffer[255];
        int nb_fautes = 0;
        int partie_finie = 0;
        char mot_courant[255] = "";

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

            if (buffer[0] == 'P') // Format: P:nb_fautes:mot_cache
            {
                sscanf(buffer, "P:%d:%[^\n]", &nb_fautes, mot_courant);
                
                draw_pendu(nb_fautes, strlen(mot_courant));
                printf("Mot à deviner : %s\n", mot_courant);
                printf("Nombre de fautes : %d/%d\n\n", nb_fautes, ERREURS_MAX);
                
                // Vérifier si le mot est complet (sans underscore)
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
            else if (buffer[0] == 'V') // Format: V:mot_complet
            { 
                char mot_final[255];
                sscanf(buffer, "V:%[^\n]", mot_final);
                
                printf("\n=================================\n");
                printf("🎉 VICTOIRE ! Vous avez trouvé le mot !\n");
                printf("Le mot était : %s\n", mot_final);
                printf("=================================\n");
                partie_finie = 1;
                continue;
            }
            else if (buffer[0] == 'D') // Format: D:mot_complet
            { 
                char mot_final[255];
                sscanf(buffer, "D:%[^\n]", mot_final);
                
                draw_pendu(ERREURS_MAX, strlen(mot_final));
                printf("\n=================================\n");
                printf("💀 DÉFAITE ! Vous avez été pendu !\n");
                printf("Le mot était : %s\n", mot_final);
                printf("=================================\n");
                partie_finie = 1;
                continue;
            }

            if (!partie_finie)
            {
                printf("Entrer une lettre ('.' pour quitter) : ");
                fgets(buffer, sizeof(buffer), stdin);
                buffer[strcspn(buffer,"\n")] = 0;
                
                if (strcmp(buffer,".") == 0) {
                    send(socketClient, buffer, strlen(buffer)+1, 0);
                    break;
                }
                
                send(socketClient, buffer, strlen(buffer)+1, 0);
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