#include <stdio.h>
#include <stdlib.h> /* pour exit */
#include <unistd.h> /* pour read, write, close, sleep */
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>     /* pour memset */
#include <netinet/in.h> /* pour struct sockaddr_in */
#include <arpa/inet.h>  /* pour htons et inet_aton */

#define PORT 5000 //(ports >= 5000 réservés pour usage explicite)

#define LG_MESSAGE 256
#define MAX_LETTRES 100
#define MAX_ESSAIS 6

void lire_heure(char *heure)
{
    FILE *fpipe;

    fpipe = popen("date'+%X'", "r");
    if (fpipe == NULL)
    {
        perror("popen");
        exit(-1);
    }
    fgets(heure, LG_MESSAGE, fpipe);
    pclose(fpipe);
}

void lire_date(char *date)
{
    FILE *fpipe;
    fpipe = popen("date '+%A %d %B %Y'", "r");
    if (fpipe == NULL)
    {
        perror("popen");
        exit(-1);
    }
    fgets(date, LG_MESSAGE, fpipe);
    pclose(fpipe);
}

int main(int argc, char *argv[])
{
    int socketEcoute;

    struct sockaddr_in pointDeRencontreLocal;
    socklen_t longueurAdresse;

    int socketDialogue;
    struct sockaddr_in pointDeRencontreDistant;
    char messageRecu[LG_MESSAGE]; /* le message de la couche Application ! */
    int ecrits, lus;              /* nb d’octets ecrits et lus */
    int retour;

    /*******************************CRÉATION SOCKET************************/
    // Crée un socket de communication
    socketEcoute = socket(AF_INET, SOCK_STREAM, 0);
    // Teste la valeur renvoyée par l’appel système socket()
    if (socketEcoute < 0)
    {
        perror("socket"); // Affiche le message d’erreur
        exit(-1);         // On sort en indiquant un code erreur
    }
    printf("Socket créée avec succès ! (%d)\n", socketEcoute); // On prépare l’adresse d’attachement locale
    // setsockopt()
    /*************************************************************************/

    // Remplissage de sockaddrDistant (structure sockaddr_in identifiant le point d'écoute local)
    longueurAdresse = sizeof(pointDeRencontreLocal);
    // memset sert à faire une copie d'un octet n fois à partir d'une adresse mémoire donnée
    // ici l'octet 0 est recopié longueurAdresse fois à partir de l'adresse &pointDeRencontreLocal
    /********************ATTACHEMENT D'ADRESSE LOCALE************************/
    memset(&pointDeRencontreLocal, 0x00, longueurAdresse);
    pointDeRencontreLocal.sin_family = PF_INET;
    pointDeRencontreLocal.sin_addr.s_addr = htonl(INADDR_ANY); // attaché à toutes les interfaces locales disponibles
    pointDeRencontreLocal.sin_port = htons(PORT);              // = 5000 ou plus

    // On demande l’attachement local de la socket
    if ((bind(socketEcoute, (struct sockaddr *)&pointDeRencontreLocal, longueurAdresse)) < 0)
    {
        perror("bind");
        exit(-2);
    }
    printf("Socket attachée avec succès !\n");
    /******************************************************************************* */
    // On fixe la taille de la file d’attente à 5 (pour les demandes de connexion non encore traitées)
    if (listen(socketEcoute, 5) < 0)
    {
        perror("listen");
        exit(-3);
    }
    printf("Socket placée en écoute passive ...\n");

    // c’est un appel bloquant
    socketDialogue = accept(socketEcoute, (struct sockaddr *)&pointDeRencontreDistant, &longueurAdresse);
    if (socketDialogue < 0)
    {
        perror("accept");
        close(socketDialogue);
        close(socketEcoute);
        exit(-4);
    }

    const char *mot = "bonjour";
    int longueur = strlen(mot);
    char mot_cache[MAX_LETTRES];
    int lettres_trouvees[MAX_LETTRES] = {0};
    int essais_restants = MAX_ESSAIS;
    int partie_finie = 0;

    // Initialiser le mot caché
    for (int i = 0; i < longueur; i++)
        mot_cache[i] = '_';
    mot_cache[longueur] = '\0';

    // boucle d’attente de connexion : en théorie, un serveur attend indéfiniment !
    // while (1)
    // {
    //     memset(messageRecu, 'a', LG_MESSAGE * sizeof(char));
    //     printf("Attente d’une demande de connexion (quitter avec Ctrl-C)\n\n");

    //     // On réception les données du client (cf. protocole)
    //     // lus = read(socketDialogue, messageRecu, LG_MESSAGE*sizeof(char)); // ici appel bloquant
    //     lus = recv(socketDialogue, messageRecu, LG_MESSAGE * sizeof(char), 0); // ici appel bloquant
    //     switch (lus)
    //     {
    //     case -1: /* une erreur ! */
    //         perror("read");
    //         close(socketDialogue);
    //         exit(-5);
    //     case 0: /* la socket est fermée */
    //         fprintf(stderr, "La socket a été fermée par le client !\n\n");
    //         close(socketDialogue);
    //         return 0;
    //     default: /* réception de n octets */
    //         printf("Message reçu : %s (%d octets)\n\n", messageRecu, lus);

    //         if (!partie_finie)
    //         {
    //             char lettre = messageRecu[0]; // on suppose que le client envoie 1 lettre
    //             int correcte = 0;
    //             int deja_trouve = 0;

    //             // Vérifier si la lettre est dans le mot
    //             for (int i = 0; i < longueur; i++)
    //             {
    //                 if (mot[i] == lettre)
    //                 {
    //                     if (lettres_trouvees[i])
    //                     {
    //                         deja_trouve = 1;
    //                     }
    //                     else
    //                     {
    //                         lettres_trouvees[i] = 1;
    //                         mot_cache[i] = lettre;
    //                         correcte = 1;
    //                     }
    //                 }
    //             }

    //             // Préparer le message de réponse
    //             char reponse[256];
    //             if (deja_trouve)
    //             {
    //                 sprintf(reponse, "Lettre déjà choisie ! Mot actuel : %s", mot_cache);
    //             }
    //             else if (correcte)
    //             {
    //                 sprintf(reponse, "Correct ! Mot actuel : %s", mot_cache);
    //             }
    //             else
    //             {
    //                 essais_restants--;
    //                 sprintf(reponse, "Incorrect ! Essais restants : %d. Mot actuel : %s", essais_restants, mot_cache);
    //             }

    //             // Vérifier victoire
    //             int gagne = 1;
    //             for (int i = 0; i < longueur; i++)
    //             {
    //                 if (!lettres_trouvees[i])
    //                     gagne = 0;
    //             }
    //             if (gagne)
    //             {
    //                 sprintf(reponse, "Félicitations ! Vous avez trouvé le mot : %s", mot);
    //                 partie_finie = 1;
    //             }

    //             // Vérifier défaite
    //             if (essais_restants <= 0)
    //             {
    //                 sprintf(reponse, "Perdu ! Le mot était : %s", mot);
    //                 partie_finie = 1;
    //             }

    //             // Envoyer la réponse au client
    //             int nbEnvoyes = send(socketDialogue, reponse, strlen(reponse) + 1, 0);
    //             if (nbEnvoyes <= 0)
    //             {
    //                 perror("send");
    //                 close(socketDialogue);
    //                 exit(-6);
    //             }
    //             printf("Réponse envoyée au client (%d octets)\n", nbEnvoyes);
    //         }

    //         if (nbEnvoyes <= 0)
    //         {
    //             perror("send");
    //             close(socketDialogue);
    //             exit(-6);
    //         }
    //         else
    //         {
    //             printf("Réponse envoyée au client (%d octets)\n\n", nbEnvoyes);
    //         }
    //         break;
    //     }
    // }

    while (1)
    {
        memset(messageRecu, 0, LG_MESSAGE);
        printf("Attente d’une demande de connexion (quitter avec Ctrl-C)\n\n");

        lus = recv(socketDialogue, messageRecu, LG_MESSAGE, 0);
        if (lus <= 0)
        { /* gérer erreur ou fermeture */
        }

        printf("Message reçu : %s (%d octets)\n\n", messageRecu, lus);

        if (!partie_finie)
        {
            char lettre = messageRecu[0]; // le caractère reçu
            int correcte = 0;
            int deja_trouve = 0;

            // Vérifier la lettre
            for (int i = 0; i < longueur; i++)
            {
                if (mot[i] == lettre)
                {
                    if (lettres_trouvees[i])
                        deja_trouve = 1;
                    else
                    {
                        lettres_trouvees[i] = 1;
                        mot_cache[i] = lettre;
                        correcte = 1;
                    }
                }
            }

            // Préparer le message
            char reponse[256];
            if (deja_trouve)
                sprintf(reponse, "Lettre déjà choisie ! Mot actuel : %s", mot_cache);
            else if (correcte)
                sprintf(reponse, "Correct ! Mot actuel : %s", mot_cache);
            else
            {
                essais_restants--;
                sprintf(reponse, "Incorrect ! Essais restants : %d. Mot actuel : %s", essais_restants, mot_cache);
            }

            // Vérifier victoire ou défaite
            int gagne = 1;
            for (int i = 0; i < longueur; i++)
                if (!lettres_trouvees[i])
                    gagne = 0;
            if (gagne)
            {
                sprintf(reponse, "Félicitations ! Vous avez trouvé le mot : %s", mot);
                partie_finie = 1;
            }
            if (essais_restants <= 0)
            {
                sprintf(reponse, "Perdu ! Le mot était : %s", mot);
                partie_finie = 1;
            }

            // Envoyer la réponse
            int nbEnvoyes = send(socketDialogue, reponse, strlen(reponse) + 1, 0);
            if (nbEnvoyes <= 0)
            {
                perror("send");
                close(socketDialogue);
                exit(-6);
            }
            printf("Réponse envoyée au client (%d octets)\n", nbEnvoyes);
        }
    }
    // On ferme la ressource avant de quitter
    close(socketEcoute);
    return 0;
}
