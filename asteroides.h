#ifndef ASTEROIDS_H
#define ASTEROIDS_H

#include <pthread.h>
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
    pthread_mutex_t mutex;  /* un mutex por asteroide */
} Asteroide;

extern Asteroide asteroides[NUM_ASTEROIDS];
void place_asteroids(char map[][WIN_WIDTH]);

#endif /* ASTEROIDS_H */