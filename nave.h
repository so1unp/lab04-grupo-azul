#ifndef NAVE_H
#define NAVE_H

#include <ncurses.h>

/*
  nave.h:
  Definicion de la estructura Nave y sus operaciones basicas.
  Esta estructura es el "estado" de una nave minera
*/
#define NUM_RECURSOS 4

/* Indices de los recursos dentro del array de cargamento */
#define IDX_DEUTERIO    0   // es el combustible que consume la nave
#define IDX_MUTEXIO     1
#define IDX_SEMAFORITA  2
#define IDX_KERNELIO    3

#define COMBUSTIBLE_INICIAL  100
#define OXIGENO_INICIAL      100
#define COSTO_MOVIMIENTO       1

/* Estructura que representa una nave espacial minera */
typedef struct {
    int  id;
    int  x; 
    int  y;
    int  combustible;
    int  oxigeno;
    int  cargamento[NUM_RECURSOS];
    int  activa;
    char simbolo;
} Nave;

#endif