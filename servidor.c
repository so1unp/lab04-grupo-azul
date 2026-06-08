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

#include <sys/mman.h>
#include <sys/stat.h>

#define WIN_WIDTH 90
#define WIN_HEIGHT 30
#define BUFF_SIZE 1024

#define RECEIVER_MESSAGE_QUEUE "/servidor_receiver"
#define SHM_MAP_PATH "/servidor_map_shm"
#define ASTEROID_SYMBOL "*"   /* replace with your unicode: e.g. "🪨" */
#define NUM_ASTEROIDS 10
typedef struct {
    WINDOW *win;
    mqd_t receiver;
    char *buff;
} ReceiverData;

typedef struct {
    WINDOW *win;
    char (*map)[WIN_WIDTH];
} MapData;

void *receive_mq(void *param);
void *print_map(void *param);
void place_asteroids(char map[][WIN_WIDTH]);

int main(int argc, char *argv[])
{
    srand(time(NULL));

    WINDOW *win;
    pthread_t t_receiver;
    pthread_t t_printer;

    char buff[BUFF_SIZE];
    char input[BUFF_SIZE];

    int shm_fd;
    size_t map_size = WIN_HEIGHT * WIN_WIDTH;
    char (*map)[WIN_WIDTH];

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

    map = mmap(NULL, map_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (map == MAP_FAILED) {
        perror("Error en mmap");
        close(shm_fd);
        shm_unlink(SHM_MAP_PATH);
        exit(1);
    }

    memset(map, ' ', map_size);
    place_asteroids(map);
    
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

    pthread_create(&t_receiver, NULL, receive_mq, (void *)&receiver_data);
    pthread_create(&t_printer, NULL, print_map, (void *)&map_data);

    /* Bucle */
    while(strcmp(input, "FIN") != 0) {
        wmove(win, 1, 2);
        box(win, 0, 0);
        wgetstr(win, input);
    }

    // Termina la ejecución del programa.
    werase(win);
    endwin();
    pthread_join(t_receiver, NULL);
    pthread_join(t_printer, NULL);

    mq_close(receiver);

    munmap(map, map_size);
    close(shm_fd);
    shm_unlink(SHM_MAP_PATH);

    exit(EXIT_SUCCESS);
}

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
        sleep(1);
    }

    return NULL;
}

void place_asteroids(char map[][WIN_WIDTH])
{
    int placed = 0;

    while (placed < NUM_ASTEROIDS) {
        /* Random coords avoiding borders (row 0, WIN_HEIGHT-1, col 0, WIN_WIDTH-1) */
        int row = 1 + rand() % (WIN_HEIGHT - 2);
        int col = 1 + rand() % (WIN_WIDTH  - 2);

        /* Only place if the cell is empty */
        if (map[row][col] == ' ') {
            map[row][col] = '*';   /* swap '*' for your unicode char if needed */
            placed++;
        }
    }
}