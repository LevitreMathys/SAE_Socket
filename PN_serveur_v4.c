#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h> 

#define PORT 5000

// #####################
// # DÉFINITION STRUCT # 
// #####################
struct roleJeu{
    int numRole; // 0 = maitre du jeu | 1 = joueur 
}; 

int main(){
    // ################################
    // # DÉCLARATION / INITIALISATION # 
    // ################################
    struct sockaddr_in adresseServeur, adresseClient;
    struct sockaddr_in infoClient1, infoClient2;
    socklen_t tailleAdresseClient = sizeof(adresseClient); 


    // ##############################
    // # CRÉATION DU SOCKET SERVEUR # 
    // ############################## 
    int socketServeur = socket(AF_INET, SOCK_STREAM, 0);

    if (socketServeur < 0){ 
        perror("socket");
        exit(-4); 
    }
    
    memset(&adresseServeur, 0, sizeof(adresseServeur)); 
    adresseServeur.sin_family = AF_INET;
    adresseServeur.sin_addr.s_addr = htonl(INADDR_ANY);
    adresseServeur.sin_port = htons(PORT);

    int opt = 1;
    setsockopt(socketServeur, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    if (bind(socketServeur, (struct sockaddr *)&adresseServeur, sizeof(adresseServeur)) < 0){
        perror("bind");
        exit(-5);
    }

    if (listen(socketServeur, 3) < 0){
        perror("listen");
        exit(-6);
    }

    printf("Socket serveur créée, serveur prêt a recevoir des connexion\n");

    while(1){ // boucle pour toujours attendre des client 
        printf("En attente de connexion de deux clients...\n");


        // ###############################
        // # CONNEXION DU PREMIER CLIENT #
        // ###############################
        int socketClient1 = accept(socketServeur, (struct sockaddr *)&infoClient1, &tailleAdresseClient);
        
        if (socketClient1 < 0){
            perror("socket client1");
            exit(-7);
        }
        printf("Client 1 connecté.\n");


        // ################################
        // # CONNEXION DU DEUXIEME CLIENT #
        // ################################ 
        int socketClient2 = accept(socketServeur, (struct sockaddr *)&infoClient2, &tailleAdresseClient);
        
        if (socketClient2 < 0){
            perror("socket client2");
            exit(-7);
        }
        printf("Client 2 connecté.\n");


        // ########################### 
        // # ATTRIBUTION RÔLE CLIENT # 
        // ###########################
        /* envoyer au client1 que c'est le maitre du jeu et au client 2 que c'est le joueur 

        REMARQUE TRÈS TRÈS IMPORTANTE : cette partie doit être la même que celle de la V2 !!!!


        l'information qui dit maitre du jeu ou joueur : une struct avec un int dedant !! 
        
            1 = maitre du jeu 
            2 = joueur 
        
        remarque : peut-être que ça va faire la merde lool, faire des struct dans le pire des cas 
        */

        // client1 : maitre du jeu 
        struct roleJeu role_maitre_du_jeu; 
        role_maitre_du_jeu.numRole = 0; 

        send(socketClient1, &role_maitre_du_jeu, sizeof(role_maitre_du_jeu), 0); 
        
        // client2 joueur 
        struct roleJeu role_joueur; 
        role_joueur.numRole = 1; 

        send(socketClient2, &role_joueur, sizeof(role_joueur), 0); 

        printf("Rôles attribués aux clients.\n"); 


        // #########################
        // # TRANSFERT INFO CLIENT #
        // ######################### 
        /* le client1 doit avoir les info du client2 pour pouvoir s'y connecter et inversement 
        
        les info pour ce connecter dans un socket sont dans un struct sockaddr_in 

        il suffie de donner ce sockaddr_in pour pouvoir ce connecter a ce socket. 
        */ 
        // info du client2 qu'on envoie au client1 
        send(socketClient1, &infoClient2, sizeof(infoClient2), 0); 

        // info du client1 qu'on envoie au client2 
        send(socketClient2, &infoClient1, sizeof(infoClient1), 0); 


        printf("Info des clients transmises.\n");
        printf("Le serveur a fini, il attend 2 nouvelles connexion. \n"); 

        // fermeture des socketClient1 et 2 : on attend des nouveau client 
        close(socketClient1);
        memset(&socketClient1, 0, sizeof(socketClient1)); 

        close(socketClient2);
        memset(&socketClient2, 0, sizeof(socketClient2));    
    }

    close(socketServeur); 
    return 0;
}
