#include <stdio.h>
#include "game_pendu.h"

char *getMotCache(Game *game)
{
    return game->mot_cache;
}

void setNbLetres(Game *game, int nbLettres)
{
    game->longueur = nbLettres;
}

void init_word(Game *game, int nb_lettres)
{
    game->longueur = nb_lettres;

    for (int i = 0; i < game->longueur; i++)
    {
        game->mot_cache[i] = '_';
        game->lettres_trouvees[i] = 0;
    }
    game->mot_cache[game->longueur] = '\0';
    printf("Mot initialisé avec %d lettres.\n", nb_lettres);
}

void print_word(Game *game)
{
    printf("%s\n", game->mot_cache);
}
