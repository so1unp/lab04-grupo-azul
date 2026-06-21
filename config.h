#include "nave.h"
#include "estacion.h"
#ifndef CONFIG_H
#define CONFIG_H

#define RECEIVER_MESSAGE_QUEUE "/azul_servidor_receiver"
#define SHM_MAP_PATH "/azul_servidor_map_shm"

#define WIN_WIDTH  90
#define WIN_HEIGHT 30

#define WIN_ALERT_WIDTH 40
#define WIN_ALERT_HEIGHT 7

#define NUM_STATIONS  2
#define NUM_ASTEROIDS 10
#define MAX_NAVES     4


#define price_mutexio       15
#define price_semaforita    40
#define price_kernelio      55

#define price_oxigeno       20
#define price_reparar       25
#define price_condimento    500
#define price_super_armadura 50



#define COSTO_MOVIMIENTO 1
#define COSTO_EXTRACCION 5

typedef struct {
    char map[WIN_HEIGHT][WIN_WIDTH];
    Nave naves[MAX_NAVES];
    Estacion estaciones[NUM_STATIONS];
} EspacioCompartido;

#endif /* CONFIG_H */