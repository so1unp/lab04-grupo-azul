/*
 * prueba.c
 * --------
 * Programa de prueba: crea un mapa vacio y una nave para verificar
 * que la estructura "Nave" funciona correctamente.
 *
 * Controles:
 *   w / a / s / d : mover nave
 *   q             : salir
 *
 * NOTA: Esto NO es todavia el cliente final. Aca el mapa es local
 * (un array en la memoria del proceso). En el lab posta el mapa va a
 * vivir en una memoria compartida POSIX administrada por el servidor.
 * El objetivo de este programa es solamente probar que la nave se crea,
 * se dibuja y se mueve bien.
 *
 * Compilar:
 *   make
 * o a mano:
 *   gcc -Wall -Wextra -std=c11 prueba.c nave.c -o prueba -lncurses
 */

#include <stdio.h>
#include <stdlib.h>
#include <ncurses.h>
#include "nave.h"

#define ANCHO_MAPA 40
#define ALTO_MAPA  15

/* Llena el mapa con '.' (espacio vacio) */
static void inicializar_mapa(char mapa[ALTO_MAPA][ANCHO_MAPA])
{
    for (int y = 0; y < ALTO_MAPA; y++) {
        for (int x = 0; x < ANCHO_MAPA; x++) {
            mapa[y][x] = '.';
        }
    }
}

/* Dibuja el mapa, la nave encima, y la info al costado */
static void dibujar(char mapa[ALTO_MAPA][ANCHO_MAPA], const Nave *nave)
{
    clear();

    /* Dibujar el mapa */
    for (int y = 0; y < ALTO_MAPA; y++) {
        for (int x = 0; x < ANCHO_MAPA; x++) {
            if (x == nave->x && y == nave->y) {
                /* En la celda de la nave dibujamos su simbolo en lugar del mapa */
                mvaddch(y, x, nave->simbolo);
            } else {
                mvaddch(y, x, mapa[y][x]);
            }
        }
    }

    /* Panel de informacion a la derecha del mapa */
    int info_x = ANCHO_MAPA + 2;
    mvprintw(0, info_x, " Nave %d", nave->id);
    mvprintw(1, info_x, "Posicion:    (%d, %d)   ", nave->x, nave->y);
    mvprintw(2, info_x, "Combustible: %d   ", nave->combustible);
    mvprintw(3, info_x, "Oxigeno:     %d   ", nave->oxigeno);
    mvprintw(4, info_x, "Estado:      %s",
             nave->activa ? "ACTIVA      " : "DESACTIVADA ");

    /* Ayuda abajo del mapa */
    mvprintw(ALTO_MAPA + 1, 0,
             "Controles: w/a/s/d para mover, q para salir");

    refresh();
}

int main(void)
{
    /* 1) Crear el mapa vacio */
    char mapa[ALTO_MAPA][ANCHO_MAPA];
    inicializar_mapa(mapa);

    /* 2) Crear la nave en el centro del mapa */
    Nave *nave = nave_crear(1, ANCHO_MAPA / 2, ALTO_MAPA / 2);
    if (nave == NULL) {
        fprintf(stderr, "Error: no se pudo crear la nave\n");
        return 1;
    }

    /* 3) Inicializar ncurses */
    initscr();              /* arrancar modo ncurses */
    cbreak();               /* leer teclas sin esperar Enter */
    noecho();               /* no mostrar las teclas tipeadas */
    keypad(stdscr, TRUE);   /* habilitar teclas especiales */
    curs_set(0);            /* ocultar el cursor */

    /* 4) Bucle principal del juego */
    int salir = 0;
    while (!salir) {
        dibujar(mapa, nave);

        int tecla = getch();
        switch (tecla) {
            case 'w': nave_mover(nave,  0, -1, ANCHO_MAPA, ALTO_MAPA); break;
            case 's': nave_mover(nave,  0,  1, ANCHO_MAPA, ALTO_MAPA); break;
            case 'a': nave_mover(nave, -1,  0, ANCHO_MAPA, ALTO_MAPA); break;
            case 'd': nave_mover(nave,  1,  0, ANCHO_MAPA, ALTO_MAPA); break;
            case 'q': salir = 1; break;
            default:  break;  /* ignorar otras teclas */
        }
    }

    /* 5) Cerrar ncurses para volver a la terminal normal */
    endwin();

    /* 6) Mostrar el estado final por stdout y liberar memoria */
    printf("\n--- Estado final ---\n");
    nave_mostrar_info(nave);
    nave_destruir(nave);

    return 0;
}