/* ── asteroids.h ── */

#ifndef ASTEROIDS_H
#define ASTEROIDS_H

#include "config.h"   /* WIN_WIDTH is now defined before it's used */

#define NUM_ASTEROIDS 10

typedef struct {
    int row;
    int col;
    int deuterio;
    int mutexio;
    int semaforita;
    int kernelio;
    int active;
} Asteroid;

extern Asteroid asteroids[NUM_ASTEROIDS];

void place_asteroids(char map[][WIN_WIDTH]);

#endif /* ASTEROIDS_H */