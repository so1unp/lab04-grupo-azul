#ifndef ESTACION_H
#define ESTACION_H

#include <ncurses.h>
#define NUM_RECURSOS 4

/* Indices de los recursos dentro del array de cargamento */
#define IDX_DEUTERIO    0   // es el combustible que consume la nave
#define IDX_MUTEXIO     1
#define IDX_SEMAFORITA  2
#define IDX_KERNELIO    3
#define HANGAR 3

#define COMBUSTIBLE_INICIAL  100
#define OXIGENO_INICIAL_ESTACION      50
#define BILLETERA_INICIAL   0

#define ALERTA_COMBUSTIBLE 50
typedef struct {
    int  id = -1;
    int  x; 
    int  y;
    int  combustible;
    int  oxigeno;
    int  cargamento[NUM_RECURSOS];
    int  activa;
    char simbolo;
    int  hangar[HANGAR];
    int  billetera;
    bool alerta;
    pthread_mutex_t mutex;  /* un mutex por estacion */
    
} Estacion;

#endif