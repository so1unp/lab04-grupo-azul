#include <stdio.h>
#include <stdlib.h>
#include <ncurses.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <pthread.h>
#include <sys/stat.h>
#include <unistd.h>

#include "nave.h"

#define WIN_WIDTH 90
#define WIN_HEIGHT 30
#define SHM_MAP_PATH "/servidor_map_shm"

Nave *crear(int id, int x_inicial, int y_inicial);
void destruir(Nave *nave);
void mostrar_info(WINDOW *win, const Nave *nave);
int mover(Nave *nave, int dx, int dy, int ancho_mapa, int alto_mapa);
int consumir_combustible(Nave *nave, int cantidad);
void *consumir_oxigeno(void *param);

typedef struct {
    Nave *nave;
    WINDOW *win;
} nave_data;


static void dibujar(WINDOW *map_win, WINDOW *info_win, char mapa[WIN_HEIGHT][WIN_WIDTH], const Nave *nave) {
    werase(map_win);
    box(map_win, 0, 0);

    for (int y = 1; y < WIN_HEIGHT - 1; y++) {
        for (int x = 1; x < WIN_WIDTH - 1; x++) {
            if (x == nave->x && y == nave->y) {
                mvwaddch(map_win, y, x, nave->simbolo);
            } else {
                mvwaddch(map_win, y, x, mapa[y][x]);
            }
        }
    }
    wrefresh(map_win);

    mostrar_info(info_win, nave);
}

int main(void) {
    int shm_fd;
    size_t map_size = WIN_HEIGHT * WIN_WIDTH;
    char (*mapa)[WIN_WIDTH];

    pthread_t t_oxygen;

    // File Descriptor de memoria compartida
    shm_fd = shm_open(SHM_MAP_PATH, O_RDWR, 0);
    if (shm_fd < 0) {
        perror("Error al acceder a la memoria compartida del servidor");
        return 1;
    }

    // Mapea la memoria compartida con la del servidor
    mapa = mmap(NULL, map_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (mapa == MAP_FAILED) {
        perror("Error en mmap");
        close(shm_fd);
        return 1;
    }

    // Inicializa Ncurses
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);

    WINDOW *map_win = newwin(WIN_HEIGHT, WIN_WIDTH, 0, 0);
    WINDOW *info_win = newwin(14, 35, 0, WIN_WIDTH + 2);

    Nave *nave = crear(1, WIN_WIDTH / 2, WIN_HEIGHT / 2);
    if (nave == NULL) {
        endwin();
        munmap(mapa, map_size);
        close(shm_fd);
        fprintf(stderr, "Error: no se pudo crear la nave\n");
        exit(1);
    }

    nave_data nave_data = {
        .nave = nave,
        .win = info_win
    };

    pthread_create(&t_oxygen, NULL, consumir_oxigeno, (void *)&nave_data);

    int salir = 0;
    while (!salir) {
        dibujar(map_win, info_win, mapa, nave);

        int tecla = wgetch(map_win);
        int dx = 0, dy = 0;

        switch (tecla) {
            case 'w': dy = -1; break;
            case 's': dy = 1; break;
            case 'a': dx = -1; break;
            case 'd': dx = 1; break;
            case 'q': salir = 1; break;
            default: break;
        }

        if ((dx != 0 || dy != 0) && nave->activa) {
            mover(nave, dx, dy, WIN_WIDTH, WIN_HEIGHT);
        }
    }

    werase(map_win);
    werase(info_win);
    endwin();

    destruir(nave);
    munmap(mapa, map_size);
    close(shm_fd);

    pthread_join(t_oxygen, NULL);
    return 0;
}

Nave *crear(int id, int x_inicial, int y_inicial) {
    Nave *nave = malloc(sizeof(Nave));
    if (nave == NULL) {
        perror("malloc nave");
        return NULL;
    }

    nave->id          = id;
    nave->x           = x_inicial;
    nave->y           = y_inicial;
    nave->combustible = COMBUSTIBLE_INICIAL;
    nave->oxigeno     = OXIGENO_INICIAL;
    nave->activa      = 1;
    nave->simbolo     = 'N';

    for (int i = 0; i < NUM_RECURSOS; i++) {
        nave->cargamento[i] = 0;
    }

    return nave;
}

void destruir(Nave *nave) {
    if (nave != NULL) {
        free(nave);
    }
}

void mostrar_info(WINDOW *win, const Nave *nave) {
    if (win == NULL || nave == NULL) {
        return;
    }

    werase(win);
    box(win, 0, 0);

    mvwprintw(win, 1, 2, "Nave %d", nave->id);
    mvwprintw(win, 2, 2, "Posicion:    (%d, %d)", nave->x, nave->y);
    mvwprintw(win, 3, 2, "Combustible: %d", nave->combustible);
    mvwprintw(win, 4, 2, "Oxigeno:     %d", nave->oxigeno);

    if (nave->activa) {
        mvwprintw(win, 5, 2, "Estado:      ACTIVA");
    } else {
        mvwprintw(win, 5, 2, "Estado:      DESACTIVADA");
    }

    mvwprintw(win, 7, 2, "Cargamento:");
    mvwprintw(win, 8, 4, "Deuterio:   %d", nave->cargamento[IDX_DEUTERIO]);
    mvwprintw(win, 9, 4, "Mutexio:    %d", nave->cargamento[IDX_MUTEXIO]);
    mvwprintw(win, 10, 4, "Semaforita: %d", nave->cargamento[IDX_SEMAFORITA]);
    mvwprintw(win, 11, 4, "Kernelio:   %d", nave->cargamento[IDX_KERNELIO]);

    wrefresh(win);
}

int mover(Nave *nave, int dx, int dy, int ancho_mapa, int alto_mapa) {
    if (nave == NULL || !nave->activa) return 0; 

    int nueva_x = nave->x + dx;
    int nueva_y = nave->y + dy;

    if (nueva_x < 1 || nueva_x >= ancho_mapa - 1) { 
        return 0;
    }   
    if (nueva_y < 1 || nueva_y >= alto_mapa - 1) { 
        return 0;
    }

    if (!consumir_combustible(nave, COSTO_MOVIMIENTO)) {
        return 0;
    }

    nave->x = nueva_x;
    nave->y = nueva_y;
    return 1;
}

int consumir_combustible(Nave *nave, int cantidad) {
    if (nave == NULL || !nave->activa) {
        return 0;
    }
    if (nave->combustible < cantidad) {
        nave->combustible = 0;
        nave->activa = 0;
        return 0;
    }

    nave->combustible -= cantidad;
    if (nave->combustible == 0) {
        nave->activa = 0;
    }
    return 1;
}

void *consumir_oxigeno(void *param) {
    nave_data *data = (nave_data *)param;
    int cantidad = 1;

    if (data->nave == NULL || !data->nave->activa) {
        return NULL;
    }

    while(data->nave->activa) {
        sleep(1);
        if (data->nave->oxigeno < cantidad) {
            data->nave->oxigeno = 0;
            data->nave->activa = 0;
            return NULL;
        }

        data->nave->oxigeno -= cantidad;
        wrefresh(data->win);

        if (data->nave->oxigeno == 0) {
            data->nave->activa = 0;
        }
    }

    return NULL;
}