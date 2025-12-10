
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
     "  |              ",
     "  |              ",
     "  |              ",
     "__|__            "},
    {"   _________     ",
     "  |         |    ",
     "  |         O    ",
     "  |              ",
     "  |              ",
     "  |              ",
     "__|__            "},
    {"   _________     ",
     "  |         |    ",
     "  |         O    ",
     "  |         |    ",
     "  |              ",
     "  |              ",
     "__|__            "},
    {"   _________     ",
     "  |         |    ",
     "  |         O    ",
     "  |        /|    ",
     "  |              ",
     "  |              ",
     "__|__            "},
    {"   _________     ",
     "  |         |    ",
     "  |         O    ",
     "  |        /|\\   ",
     "  |              ",
     "  |              ",
     "__|__            "},
    {"   _________     ",
     "  |         |    ",
     "  |         O    ",
     "  |        /|\\   ",
     "  |        /     ",
     "  |              ",
     "__|__            "},
    {"   _________     ",
     "  |         |    ",
     "  |         O    ",
     "  |        /|\\   ",
     "  |        / \\   ",
     "  |              ",
     "__|__            "}};

// Fonction pour afficher le pendu et le mot
void draw_pendu(int nb_fautes, int len_word)
{

    if (nb_fautes < 0)
        nb_fautes = 0;
    if (nb_fautes >= NB_DESSINS)
        nb_fautes = NB_DESSINS - 1;

    for (int i = 0; i < LIGNES_PENDU; i++)
    {

        // affiche la ligne du pendu
        printf("%s\n", pendu[nb_fautes][i]);
    }

    printf("Entrez une lettre : ");
}

#endif
