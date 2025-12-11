
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "game_pendu.h"

#define PORT 5000
#define LG_MESSAGE 256

int main(int argc, char *argv[])
{
    Game game;
    game.mot = "bonjour";
    game.longueur = strlen(game.mot);
    game.essais_restants = MAX_ESSAIS;
    game.partie_finie = 0;
    memset(game.lettres_trouvees, 0, sizeof(game.lettres_trouvees));
    init_word(&game);

    int socketEcoute, socketDialogue;
    struct sockaddr_in addrLocal, addrDistant;
    socklen_t tailleAdresse = sizeof(addrLocal);

    char messageRecu[LG_MESSAGE];
    char message_a_envoyer[512];

    // Création du socket
    socketEcoute = socket(AF_INET, SOCK_STREAM, 0);
    if (socketEcoute < 0)
    {
        perror("socket");
        exit(-1);
    }

    // Réutilisation de l'adresse
    int opt = 1;
    if (setsockopt(socketEcoute, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        perror("setsockopt");
        exit(-1);
    }

    memset(&addrLocal, 0, sizeof(addrLocal));
    addrLocal.sin_family = AF_INET;
    addrLocal.sin_addr.s_addr = htonl(INADDR_ANY);
    addrLocal.sin_port = htons(PORT);

    if (bind(socketEcoute, (struct sockaddr *)&addrLocal, sizeof(addrLocal)) < 0)
    {
        perror("bind");
        exit(-2);
    }

    if (listen(socketEcoute, 5) < 0)
    {
        perror("listen");
        exit(-3);
    }

    printf("Serveur en écoute sur le port %d...\n", PORT);

    socketDialogue = accept(socketEcoute, (struct sockaddr *)&addrDistant, &tailleAdresse);
    if (socketDialogue < 0)
    {
        perror("accept");
        close(socketEcoute);
        exit(-4);
    }

    printf("Client connecté !\n");

    while (1)
    {
        memset(messageRecu, 0, LG_MESSAGE);

        int lus = recv(socketDialogue, messageRecu, LG_MESSAGE, 0);
        if (lus <= 0)
        {
            printf("Le client a fermé la connexion.\n");
            break;
        }

        printf("Message reçu : %s\n", messageRecu);

        // Quitter la connexion
        if (strcmp(messageRecu, ".") == 0)
        {
            printf("Client demande à quitter.\n");
            break;
        }

        // Nouvelle partie
        if (strcmp(messageRecu, "start x") == 0)
        {
            printf("Nouvelle partie demandée.\n");

            game.mot = "bonjour";
            game.longueur = strlen(game.mot);
            game.essais_restants = MAX_ESSAIS;
            game.partie_finie = 0; // ⚠ impératif
            memset(game.lettres_trouvees, 0, sizeof(game.lettres_trouvees));
            init_word(&game);

            snprintf(message_a_envoyer, sizeof(message_a_envoyer),
                     "--- Nouvelle partie ! ---\nMot : %s", game.mot_cache);
            send(socketDialogue, message_a_envoyer, strlen(message_a_envoyer) + 1, 0);
            continue;
        }

        // Si la partie est finie et que le client n'a pas demandé de nouvelle partie
        if (game.partie_finie)
        {
            snprintf(message_a_envoyer, sizeof(message_a_envoyer),
                     "Partie terminée. Tapez 'start x' pour recommencer ou '.' pour quitter.");
            send(socketDialogue, message_a_envoyer, strlen(message_a_envoyer) + 1, 0);
            continue;
        }

        // Traitement d'une lettre normale
        process_letter(&game, messageRecu[0], message_a_envoyer, sizeof(message_a_envoyer));
        send(socketDialogue, message_a_envoyer, strlen(message_a_envoyer) + 1, 0);
    }

    close(socketDialogue);
    close(socketEcoute);

    return 0;
}
