#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <ncurses.h>
#include <time.h>
#include <string.h>
#include <fcntl.h>
#include <mqueue.h>
#include <unistd.h>
#include <locale.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include "asteroides.h"
#include "config.h"
#include "nave.h"

#define BUFF_SIZE 1024
#define RECEIVER_MESSAGE_QUEUE "/servidor_receiver"
#define SHM_MAP_PATH "/servidor_map_shm"
#define ASTEROID_SYMBOL '*'

Asteroide asteroides[NUM_ASTEROIDS];
EspacioCompartido *espacio_compartido;

typedef struct {
    WINDOW *win;
    mqd_t receiver;
    char *buff;
} ReceiverData;

typedef struct {
    WINDOW *win;
} MapData;

void *receive_mq(void *param);
void *print_map(void *param);
void *loop_juego(void *param);
void place_asteroids(char map[][WIN_WIDTH]);

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    setlocale(LC_ALL, "");
    srand((unsigned int)time(NULL));

    WINDOW *win;
    pthread_t t_receiver, t_printer, t_game;
    char buff[BUFF_SIZE];
    size_t total_shm_size = sizeof(EspacioCompartido);

    int shm_fd;
    shm_fd = shm_open(SHM_MAP_PATH, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    if (shm_fd < 0) {
        perror("Error al crear la memoria compartida");
        exit(1);
    }

    if (ftruncate(shm_fd, (off_t)total_shm_size) == -1) {
        perror("Error en ftruncate");
        close(shm_fd);
        shm_unlink(SHM_MAP_PATH);
        exit(1);
    }

    espacio_compartido = mmap(NULL, total_shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (espacio_compartido == MAP_FAILED) {
        perror("Error en mmap");
        close(shm_fd);
        shm_unlink(SHM_MAP_PATH);
        exit(1);
    }

    memset(espacio_compartido, 0, total_shm_size);
    
    for (int y = 0; y < WIN_HEIGHT; y++) {
        for (int x = 0; x < WIN_WIDTH; x++) {
            espacio_compartido->map[y][x] = ' ';
        }
    }
    
    place_asteroids(espacio_compartido->map);
    
    mqd_t receiver;
    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = BUFF_SIZE;
    attr.mq_curmsgs = 0;

    mq_unlink(RECEIVER_MESSAGE_QUEUE);
    if ((receiver = mq_open(RECEIVER_MESSAGE_QUEUE, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR, &attr)) == -1) {
        perror("Error al abrir la cola de mensajes");
        exit(1);
    }

    initscr();
    nodelay(stdscr, TRUE);
    keypad(stdscr, TRUE);
    clear();
    refresh();

    win = newwin(WIN_HEIGHT, WIN_WIDTH, 0, 0);
    box(win, 0, 0);
    wrefresh(win);

    ReceiverData r_data = {
        .win = win,
        .receiver = receiver,
        .buff = buff
    };

    MapData m_data = {
        .win = win
    };

    pthread_create(&t_receiver, NULL, receive_mq, (void *)&r_data);
    pthread_create(&t_printer, NULL, print_map, (void *)&m_data);
    pthread_create(&t_game, NULL, loop_juego, NULL);

    while(1) {
        int ch = wgetch(win);
        if (ch == 'q' || ch == 'Q') break;
        usleep(50000);
    }

    werase(win);
    endwin();
    
    pthread_cancel(t_receiver);
    pthread_cancel(t_printer);
    pthread_cancel(t_game);
    
    pthread_join(t_receiver, NULL);
    pthread_join(t_printer, NULL);
    pthread_join(t_game, NULL);
    
    mq_close(receiver);
    mq_unlink(RECEIVER_MESSAGE_QUEUE);
    munmap(espacio_compartido, total_shm_size);
    close(shm_fd);
    shm_unlink(SHM_MAP_PATH);

    exit(EXIT_SUCCESS);
}

void *receive_mq(void *param) {
    ReceiverData *data = (ReceiverData *)param;
    unsigned int prio = 1;

    while(1) {
        memset(data->buff, 0, BUFF_SIZE);

        if (mq_receive(data->receiver, data->buff, BUFF_SIZE, &prio) == -1) {
            perror("Error al recibir el mensaje");
            return NULL;
        }

        char tipo;
        int id = 0, arg1 = 0, arg2 = 0;
        
        if (sscanf(data->buff, "%c %d %d %d", &tipo, &id, &arg1, &arg2) >= 2) {
            if (id >= 0 && id < MAX_NAVES) {
                if (tipo == 'I') {
                    if (!espacio_compartido->naves[id].activa) {
                        espacio_compartido->naves[id].id = id;
                        espacio_compartido->naves[id].x = WIN_WIDTH / 2;
                        espacio_compartido->naves[id].y = WIN_HEIGHT / 2;
                        espacio_compartido->naves[id].combustible = COMBUSTIBLE_INICIAL;
                        espacio_compartido->naves[id].oxigeno = OXIGENO_INICIAL;
                        espacio_compartido->naves[id].activa = 1;
                        espacio_compartido->naves[id].simbolo = 'N';
                        espacio_compartido->map[espacio_compartido->naves[id].y][espacio_compartido->naves[id].x] = 'N';
                    }
                } 
                else if (tipo == 'M' && espacio_compartido->naves[id].activa) {
                    int dx = arg1;
                    int dy = arg2;
                    int nx = espacio_compartido->naves[id].x + dx;
                    int ny = espacio_compartido->naves[id].y + dy;

                    if (nx >= 1 && nx < WIN_WIDTH - 1 && ny >= 1 && ny < WIN_HEIGHT - 1) {
                        if (espacio_compartido->map[ny][nx] == ' ') {
                            if (espacio_compartido->naves[id].combustible >= COSTO_MOVIMIENTO) {
                                espacio_compartido->map[espacio_compartido->naves[id].y][espacio_compartido->naves[id].x] = ' ';
                                espacio_compartido->naves[id].x = nx;
                                espacio_compartido->naves[id].y = ny;
                                espacio_compartido->naves[id].combustible -= COSTO_MOVIMIENTO;
                                espacio_compartido->map[ny][nx] = 'N';
                                if (espacio_compartido->naves[id].combustible == 0) {
                                    espacio_compartido->naves[id].activa = 0;
                                    espacio_compartido->map[ny][nx] = ' ';
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return NULL;
}

void *print_map(void *param) {
    MapData *data = (MapData *)param;

    while(1) {
        for (int i = 0; i < WIN_HEIGHT; i++) {
            for (int i_x = 0; i_x < WIN_WIDTH; i_x++) {
                wmove(data->win, i, i_x);
                wprintw(data->win, "%c", espacio_compartido->map[i][i_x]);
            }
        }
        box(data->win, 0, 0);
        wrefresh(data->win);
        usleep(100000);
    }
    return NULL;
}

void *loop_juego(void *param) {
    (void)param;
    while(1) {
        sleep(1);
        for (int i = 0; i < MAX_NAVES; i++) {
            if (espacio_compartido->naves[i].activa) {
                if (espacio_compartido->naves[i].oxigeno > 0) {
                    espacio_compartido->naves[i].oxigeno--;
                    if (espacio_compartido->naves[i].oxigeno == 0) {
                        espacio_compartido->naves[i].activa = 0;
                        espacio_compartido->map[espacio_compartido->naves[i].y][espacio_compartido->naves[i].x] = ' ';
                    }
                }
            }
        }
    }
    return NULL;
}

void place_asteroids(char map[][WIN_WIDTH]) {
    int placed = 0;
    while (placed < NUM_ASTEROIDS) {
        int row = 1 + rand() % (WIN_HEIGHT - 2);
        int col = 1 + rand() % (WIN_WIDTH - 2);

        if (map[row][col] != ' ') continue;

        Asteroide *asteroide = &asteroides[placed];
        asteroide->row = row;
        asteroide->col = col;
        asteroide->active = 1;
        asteroide->deuterio = 20 + rand() % 31;
        asteroide->mutexio = (rand() % 2) ? (5 + rand() % 6) : 0;
        asteroide->semaforita = (rand() % 2) ? (3 + rand() % 6) : 0;
        asteroide->kernelio = (rand() % 2) ? (1 + rand() % 3) : 0;
        asteroide->oxigeno = (rand() % 5 == 0) ? (1 + rand() % 5) : 0;
        asteroide->aleacion = (rand() % 5 == 0) ? (1 + rand() % 3) : 0;
        asteroide->condimento = (rand() % 5 == 0) ? (1 + rand() % 4) : 0;

        map[row][col] = ASTEROID_SYMBOL;
        placed++;
    }
}