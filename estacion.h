#ifndef ESTACION_H
#define ESTACION_H

#include <ncurses.h>

typedef struct {
    int  id;
    int  x; 
    int  y;
    int  combustible;
    int  oxigeno;
} Estacion;

#endif