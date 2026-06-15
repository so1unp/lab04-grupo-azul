/* ── asteroids.h ── */
#ifndef ASTEROIDS_H
#define ASTEROIDS_H
#include "config.h"

typedef struct {
    int row;
    int col;
    int deuterio;
    int mutexio;
    int semaforita;
    int kernelio;
    int oxigeno; 
    int aleacion; 
    int condimento;
    int active;
} Asteroide;

extern Asteroide asteroides[NUM_ASTEROIDS];
void place_asteroids(char map[][WIN_WIDTH]);
#endif /* ASTEROIDES_H */