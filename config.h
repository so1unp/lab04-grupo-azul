#include "nave.h"

#ifndef CONFIG_H
#define CONFIG_H

/* Nombres de las colas de mensajes y memoria compartida */
#define RECEIVER_MESSAGE_QUEUE "/azul_servidor_receiver"
#define SHM_MAP_PATH "/azul_servidor_map_shm"

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
#define COSTO_EXTRACCION  5

typedef struct {
    char map[WIN_HEIGHT][WIN_WIDTH];
    Nave naves[MAX_NAVES];
    pthread_mutex_t  mutex_extraccion;
} EspacioCompartido;

#endif /* CONFIG_H */