#ifndef GAME_PENDU_H
#define GAME_PENDU_H

#include <stdio.h>

#define MAX_LETTRES 100
#define MAX_ESSAIS 6

// Structure représentant l'état d'une partie de pendu
typedef struct
{
    char mot_cache[MAX_LETTRES];       // Mot affiché au joueur avec _ pour lettres non trouvées
    const char *mot;                   // Mot à deviner
    int lettres_trouvees[MAX_LETTRES]; // Lettres déjà trouvées
    int essais_restants;               // Nombre d'essais restants
    int longueur;                      // Longueur du mot
    int partie_finie;                  // 0 = en cours, 1 = fin de partie
} Game;

// Initialisation du mot caché
void init_word(Game *game);

// Vérifier si la lettre est dans le mot
void letter_in_word(Game *game, char lettre, int *correcte, int *deja_trouve);

// Vérifier si la partie est gagnée ou perdue et mettre à jour le message
void check_result(Game *game, char *message, size_t taille_buffer, int nb_fautes);

// Construire le message à envoyer au client
void informer_user(Game *game, char *message, size_t taille_buffer, int correcte, int deja_trouve, int nb_fautes);
void process_letter(Game *game, char lettre, char *message, size_t taille_buffer);

#endif
