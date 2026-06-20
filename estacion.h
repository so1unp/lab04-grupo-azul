#ifndef ESTACION_H
#define ESTACION_H

#include <ncurses.h>
#define NUM_RECURSOS 4

/* Indices de los recursos dentro del array de cargamento */
#define IDX_DEUTERIO    0   // es el combustible que consume la nave
#define IDX_MUTEXIO     1
#define IDX_SEMAFORITA  2
#define IDX_KERNELIO    3


#define COMBUSTIBLE_INICIAL  0
#define OXIGENO_INICIAL      100
#define CONSUMO_OXIGENO      1

typedef struct {
    int  id;
    int  x; 
    int  y;
    int  combustible;
    int  oxigeno;
    int  cargamento[NUM_RECURSOS];
    int  activa;
    char simbolo;
    pthread_mutex_t mutex;  /* un mutex por estacion */
} Estacion;

#endif