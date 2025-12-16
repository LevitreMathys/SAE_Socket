
#ifndef PENDU_ASCII_H
#define PENDU_ASCII_H

#include <stdio.h>

#define LIGNES_LOGO 7
#define TAILLE_LIGNE_LOGO 100

#define NB_DESSINS 7
#define LIGNES_PENDU 7
#define TAILLE_LIGNE 50

// Dessins du pendu, lignes parfaitement alignées
extern const char pendu[NB_DESSINS][LIGNES_PENDU][TAILLE_LIGNE];
// Fonction pour afficher le pendu et le mot
void draw_pendu(int nb_fautes, int len_word);
void draw_logo();

#endif
