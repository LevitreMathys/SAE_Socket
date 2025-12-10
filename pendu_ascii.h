
#ifndef PENDU_ASCII_H
#define PENDU_ASCII_H

#include <stdio.h>

#define NB_DESSINS 7
#define LIGNES_PENDU 7
#define TAILLE_LIGNE 50

// Dessins du pendu, lignes parfaitement alignées
const char pendu[NB_DESSINS][LIGNES_PENDU][TAILLE_LIGNE] = {
    {"   _________     ",
     "  |         |    ",
     "  |              ",
     " |              ",
     "  |              ",
     "  |              ",
     "__|__            "},
    {"   _________     ",
     "  |         |    ",
     "  |         O    ",
     " |              ",
     "  |              ",
     "  |              ",
     "__|__            "},
    {"   _________     ",
     "  |         |    ",
     "  |         O    ",
     " |         |    ",
     "  |              ",
     "  |              ",
     "__|__            "},
    {"   _________     ",
     "  |         |    ",
     "  |         O    ",
     " |        /|    ",
     "  |              ",
     "  |              ",
     "__|__            "},
    {"   _________     ",
     "  |         |    ",
     "  |         O    ",
     " |        /|\\   ",
     "  |              ",
     "  |              ",
     "__|__            "},
    {"   _________     ",
     "  |         |    ",
     "  |         O    ",
     " |        /|\\   ",
     "  |        /     ",
     "  |              ",
     "__|__            "},
    {"   _________     ",
     "  |         |    ",
     "  |         O    ",
     " |        /|\\   ",
     "  |        / \\   ",
     "  |              ",
     "__|__            "}};

// Fonction pour afficher le pendu et le mot
void draw_pendu(int nb_fautes, int len_word)
{
    int espacement = 10; // espace à gauche du pendu

    if (nb_fautes < 0)
        nb_fautes = 0;
    if (nb_fautes >= NB_DESSINS)
        nb_fautes = NB_DESSINS - 1;

    for (int i = 0; i < LIGNES_PENDU; i++)
    {
        if (i != 3)
        {
            // affiche l'espace avant chaque ligne
            for (int j = 0; j < espacement; j++)
            {
                printf(" ");
            }
        }

        // si on est à la 4ème ligne (indice 3), affiche les underscores du mot avant le pendu
        if (i == 3)
        {
            for (int k = 0; k < len_word; k++)
                printf("_ ");
            printf(" "); // petit espace entre le mot et le pendu
        }

        // affiche la ligne du pendu
        printf("%s\n", pendu[nb_fautes][i]);
    }

    printf("Entrez une lettre : ");
}

#endif
