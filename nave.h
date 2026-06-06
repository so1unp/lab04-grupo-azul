#ifndef NAVE_H
#define NAVE_H

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

// aca estan las funciones para crear, destruir, mostrar info, mover, y su consumicion tambien claro.

// Crea una nueva nave con valores iniciales. Devuelve NULL si falla malloc.
Nave *nave_crear(int id, int x_inicial, int y_inicial);

// Libera la memoria reservada para la nave
void nave_destruir(Nave *nave);

// Imprime por stdout la informacion actual de la nave
void nave_mostrar_info(const Nave *nave);

//movimiento de la nave, devuelve 1 si se movio, 0 por falta de combustible
int nave_mover(Nave *nave, int dx, int dy, int ancho_mapa, int alto_mapa);

//Resta combustible. Si llega a 0, la nave queda desactivada
int nave_consumir_combustible(Nave *nave, int cantidad);

//Resta oxigeno. Si llega a 0, la nave queda desactivada y no se mueve mas
int nave_consumir_oxigeno(Nave *nave, int cantidad);

#endif // NAVE_H fin de nave.h
