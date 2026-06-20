#include <stdio.h>
#include <stdlib.h>
#include <ncurses.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <mqueue.h>

#include "nave.h"
#include "config.h"

static void dibujar(WINDOW *map_win, EspacioCompartido *espacio_compartido, int my_id) {
    werase(map_win);
    box(map_win, 0, 0);

    for (int y = 1; y < WIN_HEIGHT - 1; y++) {
        for (int x = 1; x < WIN_WIDTH - 1; x++) {
            mvwaddch(map_win, y, x, espacio_compartido->map[y][x]);
        }
    }
    wrefresh(map_win);

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
    wtimeout(map_win, 100);

    int salir = 0;
    while (!salir) {
        dibujar(map_win, espacio_compartido, my_id);
        
        int tecla = wgetch(map_win);
        
        if (tecla == 'q' || tecla == 'Q') break;
        
    }

    werase(map_win);
    endwin();

    mq_close(sender);
    munmap(espacio_compartido, total_shm_size);
    close(shm_fd);

    exit(EXIT_SUCCESS);
}
