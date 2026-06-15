#include "nave.h"

#ifndef CONFIG_H
#define CONFIG_H

/* Parámetros pantalla */
#define WIN_WIDTH 90
#define WIN_HEIGHT 30

/* Máximo 3 */
#define NUM_STATIONS 2
/* Máximo 15 */
#define NUM_ASTEROIDS 10
/* Máximo 10 */
#define MAX_NAVES 4

/* Precios combustible y minerales */
#define price_deuterio 30
#define price_mutexio 15
#define price_semaforita 40
#define price_kernelio 55

#define price_oxigeno 20

typedef struct {
    char map[WIN_HEIGHT][WIN_WIDTH];
    Nave naves[MAX_NAVES];
} EspacioCompartido;

#endif /* CONFIG_H */