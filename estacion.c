#include <stdio.h>
#include <stdlib.h>
#include <ncurses.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <mqueue.h>

#include "estacion.h"
#include "config.h"

void mostrar_info(WINDOW *win, const Estacion *estacion);

static void dibujar(WINDOW *map_win, WINDOW *info_win, EspacioCompartido *espacio_compartido, int my_id) {
    werase(map_win);
    box(map_win, 0, 0);

    for (int y = 1; y < WIN_HEIGHT - 1; y++) {
        for (int x = 1; x < WIN_WIDTH - 1; x++) {
            mvwaddch(map_win, y, x, espacio_compartido->map[y][x]);
        }
    }
    wrefresh(map_win);

    mostrar_info(info_win, &espacio_compartido->estaciones[my_id]);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Uso: %s <id_estacion> (0-%d)\n", argv[0], NUM_STATIONS - 1);
        return 1;
    }
    int my_id = atoi(argv[1]);
    if (my_id < 0 || my_id >= NUM_STATIONS) {
        printf("ID de estacion invalido (0-%d)\n", NUM_STATIONS - 1);
        return 1;
    }

    int shm_fd;
    size_t total_shm_size = sizeof(EspacioCompartido);
    EspacioCompartido *espacio_compartido;

    shm_fd = shm_open(SHM_MAP_PATH, O_RDWR, 0);
    if (shm_fd < 0) {
        perror("Error al acceder a la memoria compartida del servidor");
        return 1;
    }

    espacio_compartido = mmap(NULL, total_shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (espacio_compartido == MAP_FAILED) {
        perror("Error en mmap");
        close(shm_fd);
        return 1;
    }

    mqd_t sender;
    if ((sender = mq_open(RECEIVER_MESSAGE_QUEUE, O_WRONLY)) == -1) {
        perror("Error al conectar con la cola del servidor");
        munmap(espacio_compartido, total_shm_size);
        close(shm_fd);
        return 1;
    }

    char msg[128];
    sprintf(msg, "I E %d", my_id);
    mq_send(sender, msg, strlen(msg), 0);

    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);

    WINDOW *map_win = newwin(WIN_HEIGHT, WIN_WIDTH, 0, 0);
    WINDOW *info_win = newwin(14, 35, WIN_HEIGHT + 2, 0);
    wtimeout(map_win, 100);

    int salir = 0;
    while (!salir) {

        dibujar(map_win, info_win, espacio_compartido, my_id);
        
        int tecla = wgetch(map_win);
        
        if (tecla == 'q' || tecla == 'Q') break;

    }

    werase(map_win);
    werase(info_win);
    endwin();

    mq_close(sender);
    munmap(espacio_compartido, total_shm_size);
    close(shm_fd);

    exit(EXIT_SUCCESS);
}

void mostrar_info(WINDOW *win, const Estacion *estacion) {
    if (win == NULL || estacion == NULL) {
        return;
    }

    werase(win);
    box(win, 0, 0);

    mvwprintw(win, 1, 2, "Estacion %d", estacion->id);
    mvwprintw(win, 2, 2, "Posicion:    (%d, %d)", estacion->x, estacion->y);
    mvwprintw(win, 3, 2, "Combustible: %d", estacion->combustible);
    mvwprintw(win, 4, 2, "Oxigeno:     %d", estacion->oxigeno);
    mvwprintw(win, 5, 2, "Billetera:   %d", estacion->billetera);

    if (estacion->activa) {
        mvwprintw(win, 6, 2, "Estado:      ACTIVA");
    } else {
        mvwprintw(win, 6, 2, "Estado:      DESACTIVADA");
    }

    mvwprintw(win, 7, 2, "Para vender sus recursos presione 'v' :");
    mvwprintw(win, 8, 4, "Cambie Deuterio por Combustible");
    mvwprintw(win, 9, 4, "Mutexio:    %d", price_mutexio);
    mvwprintw(win, 10, 4, "Semaforita: %d", price_semaforita);
    mvwprintw(win, 11, 4, "Kernelio:   %d", price_kernelio);

    mvwprintw(win, 12, 2, "Recursos a la venta:");
    mvwprintw(win, 13, 4, "Para extraer combustible presione 'e'");
    mvwprintw(win, 14, 4, "Para Comprar Oxigeno presione '1'    %d", price_oxigeno);
    mvwprintw(win, 15, 4, "Para Reparar Armadura presione '2'   %d", price_reparar);
    mvwprintw(win, 16, 4, "Para Super Armadura presione '3':   %d", price_super_armadura);
    mvwprintw(win, 17, 4, "Para ComprarCondimento Para Pizza presione '4': %d", price_condimento);

    wrefresh(win);
}