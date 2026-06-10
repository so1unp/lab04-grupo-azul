#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <ncurses.h>
#include <time.h>
#include <string.h>
#include <fcntl.h>
#include <mqueue.h>
#include <unistd.h>
#include <time.h>
#include <locale.h>

#include <sys/mman.h>
#include <sys/stat.h>
#include "asteroids.h"
#include "config.h" 

#define BUFF_SIZE 1024

// msg queue receiver
#define RECEIVER_MESSAGE_QUEUE "/servidor_receiver"
// path de la memoria compartida
#define SHM_MAP_PATH "/servidor_map_shm"
#define ASTEROID_SYMBOL '*'

// lista de asteroides para guardar datos de recursos (con struct de asteroids.h)
Asteroid asteroids_list[NUM_ASTEROIDS];
 
// estructura de datos para el hilo receiver
typedef struct {
    WINDOW *win;
    mqd_t receiver;
    char *buff;
} ReceiverData;

// estructura de datos para el hilo que dibuja todo el mapa
typedef struct {
    WINDOW *win;
    char (*map)[WIN_WIDTH];
} MapData;

// declaración de funciones
void *receive_mq(void *param);
void *print_map(void *param);
void place_asteroids(char map[][WIN_WIDTH]);

int main(int argc, char *argv[])
{
    setlocale(LC_ALL, "");
    srand((unsigned int)time(NULL));

    // ventana principal
    WINDOW *win;

    // hilos
    pthread_t t_receiver;
    pthread_t t_printer;

    char buff[BUFF_SIZE];

    // tamaño del mapa
    size_t map_size = WIN_HEIGHT * WIN_WIDTH;
    char (*map)[WIN_WIDTH];

    // shared memory file descriptor (intenta abrir y si no la crea, con posible error)
    int shm_fd;
    shm_fd = shm_open(SHM_MAP_PATH, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    if (shm_fd < 0) {
        perror("Error al crear la memoria compartida");
        exit(1);
    }

    if (ftruncate(shm_fd, (off_t)map_size) == -1) {
        perror("Error en ftruncate");
        close(shm_fd);
        shm_unlink(SHM_MAP_PATH);
        exit(1);
    }

    // mapeo de memoria compartida al char map[][]
    map = mmap(NULL, map_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (map == MAP_FAILED) {
        perror("Error en mmap");
        close(shm_fd);
        shm_unlink(SHM_MAP_PATH);
        exit(1);
    }
    // seteo todos los valores del mapa a espacios
    memset(map, ' ', map_size);
    place_asteroids(map);
    
    // message queue
    mqd_t receiver;
    if ((receiver = mq_open (RECEIVER_MESSAGE_QUEUE,  O_RDWR)) == -1) { 
        printf("No se puede acceder a la cola de mensajes %s", RECEIVER_MESSAGE_QUEUE); 
        exit(1);
    }

    /* Inicializa Ncurses */
    initscr();
    nodelay(stdscr, TRUE);
    keypad(stdscr, TRUE);
    clear();
    refresh();

    /* Inicializa Ventana */
    win = newwin(WIN_HEIGHT, WIN_WIDTH, 0, 0);
    box(win, 0, 0);
    wrefresh(win);

    ReceiverData receiver_data = {
        .win = win,
        .receiver = receiver,
        .buff = buff
    };

    MapData map_data = {
        .win = win,
        .map = map
    };

    // lanza los hilos
    pthread_create(&t_receiver, NULL, receive_mq, (void *)&receiver_data);
    pthread_create(&t_printer, NULL, print_map, (void *)&map_data);

    /* Bucle */
    while(1) {
        wgetch(win);
    }

    // termina la ejecución del programa.
    werase(win);
    endwin();
    // termina los hilos
    pthread_join(t_receiver, NULL);
    pthread_join(t_printer, NULL);
    // cierro la msg queue
    mq_close(receiver);
    // desaloque la memoria compartida
    munmap(map, map_size);
    close(shm_fd);
    shm_unlink(SHM_MAP_PATH);

    exit(EXIT_SUCCESS);
}

// función para recibir mensajes de la cola
void *receive_mq(void *param) {
    ReceiverData *data = (ReceiverData *)param;
    unsigned int prio = 1;
    static int row = 1;

    while(1) {
        memset(data->buff, 0, BUFF_SIZE);

        if (mq_receive(data->receiver, data->buff, BUFF_SIZE, &prio) == -1) {
            perror("Error al recibir el mensaje");
            return NULL;
        }

        wmove(data->win, row, 2);
        wprintw(data->win, "%s", data->buff);
        box(data->win, 0, 0);
        wrefresh(data->win);

        row++;
        if (row > WIN_HEIGHT - 2) row = 1;
    }
    
    return NULL;
}

// función para imprimir la pantalla constantemente
void *print_map(void *param) {
    MapData *data = (MapData *)param;

    while(1) {
        for (int i = 0; i < WIN_HEIGHT; i++) {
            for (int j = 0; j < WIN_WIDTH; j++) {
                wmove(data->win, i, j);
                wprintw(data->win, "%c", data->map[i][j]);
            }
        }
        box(data->win, 0, 0);
        wrefresh(data->win);
        usleep(100000);
    }

    return NULL;
}
    // función para colocar en el mapa aleatoriamente asteroides

    void place_asteroids(char map[][WIN_WIDTH]) {
    // itera para imprimir la cantidad de asteroides que se le indique (NUM_ASTEROIDS)
    int placed = 0;
    while (placed < NUM_ASTEROIDS) {
        int row = 1 + rand() % (WIN_HEIGHT - 2);
        int col = 1 + rand() % (WIN_WIDTH  - 2);

        // skip si la celda no está vacia (para colisión con asteroides o estaciones)
        if (map[row][col] != ' ') continue;

        // llena el struct de asteroide
        Asteroid *a = &asteroids_list[placed];
        a->row    = row;
        a->col    = col;
        a->active = 1;

        /* deuterio: siempre presente (20-50) */
        a->deuterio = 20 + rand() % 31;

        /* mutexio, semaforita, kernelio: 50% de probabilidad cada uno */
        a->mutexio    = (rand() % 2) ? (5 + rand() % 6)  : 0;  /* 5-10 o 0 */
        a->semaforita = (rand() % 2) ? (3 + rand() % 6)  : 0;  /* 3-8  o 0 */
        a->kernelio   = (rand() % 2) ? (1 + rand() % 3)  : 0;  /* 1-3  o 0 */

        /* minerales especiales: 20% de probabilidad cada uno (más difícil) */
        a->oxigeno    = (rand() % 5 == 0) ? (1 + rand() % 5)  : 0;  /* 1-5  o 0 */
        a->aleacion   = (rand() % 5 == 0) ? (1 + rand() % 3)  : 0;  /* 1-3  o 0 */
        a->condimento = (rand() % 5 == 0) ? (1 + rand() % 4)  : 0;  /* 1-4  o 0 */

        // imprimir en el mapa
        map[row][col] = ASTEROID_SYMBOL;
        placed++;
    }

}