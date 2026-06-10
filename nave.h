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
typedef struct { //defino la estructura de la nave
    int  id;                        // identificador unica de la nave
    int  x;                         // posicion X en el mapa 
    int  y;                         // posicion Y en el mapa
    int  combustible;               // deuterio en el tanque
    int  oxigeno;                   // nivel de oxigeno
    int  cargamento[NUM_RECURSOS];  // recursos almacenados
    int  activa;                    // 1 = activa, 0 = desactivada (game over)
    char simbolo;                   // caracter que la representa en el mapa
} Nave;

#endif // NAVE_H fin de nave.h
