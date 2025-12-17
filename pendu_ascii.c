#include "pendu_ascii.h"

const char logo[LIGNES_LOGO][TAILLE_LIGNE_LOGO] = {
    "     ____.                  .___                                .___     ",
    "    |    | ____  __ __    __| _/_ __  ______   ____   ____    __| _/_ __ ",
    "    |    |/ __ \\|  |  \\  / __ |  |  \\ \\____ \\_/ __ \\ /    \\  / __ |  |  \\",
    "/\\__|    \\  ___/|  |  / / /_/ |  |  / |  |_> >  ___/|   |  \\/ /_/ |  |  /",
    "\\________|\\___  >____/  \\____ |____/  |   __/ \\___  >___|  /\\____ |____/ ",
    "              \\/             \\/       |__|        \\/     \\/      \\/      "};

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

void draw_logo()
{
    for (int i = 0; i < LIGNES_LOGO; i++)
    {
        printf("%s\n", logo[i]);
    }
}