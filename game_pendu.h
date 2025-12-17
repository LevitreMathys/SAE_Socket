#ifndef GAME_PENDU_H
#define GAME_PENDU_H

#define MAX_LETTRES 100
#define MAX_ESSAIS 6

typedef struct
{
    const char *mot;
    int longueur;
    char mot_cache[MAX_LETTRES];
    int lettres_trouvees[MAX_LETTRES];
    int essais_restants;
    int partie_finie;

} Game;

char *getMotCache(Game *game);
void setNbLetres(Game *game, int nb_lettres);
// Initialisation du mot caché
void init_word(Game *game, int nb_lettres);
void print_word(Game *game);

#endif
