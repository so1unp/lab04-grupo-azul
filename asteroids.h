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
    int oxigeno;          /* generado a partir de minerales */
    int aleacion;         /* aleaciones ultra-resistentes   */
    int condimento;       /* condimento para pizzas         */
    int active;
} Asteroid;

extern Asteroid asteroids_list[NUM_ASTEROIDS];
void place_asteroids(char map[][WIN_WIDTH]);
#endif /* ASTEROIDS_H */