#include "game_pendu.h"
#include <string.h>
#include <stdio.h>

void init_word(Game *game)
{
    for (int i = 0; i < game->longueur; i++)
        game->mot_cache[i] = '_';
    game->mot_cache[game->longueur] = '\0';
}

void letter_in_word(Game *game, char lettre, int *correcte, int *deja_trouve)
{
    for (int i = 0; i < game->longueur; i++)
    {
        if (game->mot[i] == lettre)
        {
            if (game->lettres_trouvees[i])
                *deja_trouve = 1;
            else
            {
                game->lettres_trouvees[i] = 1;
                game->mot_cache[i] = lettre;
                *correcte = 1;
            }
        }
    }
}

void check_result(Game *game, char *message, size_t taille_buffer, int nb_fautes)
{
    int gagne = 1;
    for (int i = 0; i < game->longueur; i++)
        if (!game->lettres_trouvees[i])
            gagne = 0;

    if (gagne)
    {
        snprintf(message, taille_buffer,
                 "Félicitations ! Vous avez trouvé le mot : %s | Fautes : %d",
                 game->mot, nb_fautes);
        game->partie_finie = 1;
        return;
    }

    if (game->essais_restants <= 0)
    {
        snprintf(message, taille_buffer,
                 "Perdu ! Le mot était : %s | Fautes : %d",
                 game->mot, nb_fautes);
        game->partie_finie = 1;
        return;
    }
}

void informer_user(Game *game, char *message, size_t taille_buffer, int correcte, int deja_trouve, int nb_fautes)
{
    if (deja_trouve)
    {
        snprintf(message, taille_buffer,
                 "Lettre déjà choisie ! Mot actuel : %s | Fautes : %d", game->mot_cache, nb_fautes);
    }
    else if (correcte)
    {
        snprintf(message, taille_buffer,
                 "Correct ! Mot actuel : %s | Fautes : %d", game->mot_cache, nb_fautes);
    }
    else
    {
        snprintf(message, taille_buffer,
                 "Incorrect ! Essais restants : %d. Mot actuel : %s | Fautes : %d",
                 game->essais_restants, game->mot_cache, nb_fautes);
    }
}

// Fonction pour traiter une lettre reçue et générer le message à envoyer
void process_letter(Game *game, char lettre, char *message, size_t taille_message)
{
    int correcte = 0;
    int deja_trouve = 0;

    letter_in_word(game, lettre, &correcte, &deja_trouve);

    if (!correcte && !deja_trouve)
        game->essais_restants--;

    int nb_fautes = MAX_ESSAIS - game->essais_restants;

    // Construire le message pour le client
    informer_user(game, message, taille_message, correcte, deja_trouve, nb_fautes);

    // Vérifier si la partie est gagnée ou perdue
    check_result(game, message, taille_message, nb_fautes);
}