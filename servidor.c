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

#define WIN_WIDTH 134
#define WIN_HEIGHT 25
#define BUFF_SIZE 1024

#define RECEIVER_MESSAGE_QUEUE "/servidor_receiver"

typedef struct {
    WINDOW *win;
    mqd_t receiver;
    char *buff;
} ReceiverData;

void *receive_mq(void *param);

int main(int argc, char *argv[])
{
    srand(time(NULL));

    WINDOW *win;
    pthread_t t_receiver;

    char map[WIN_HEIGHT][WIN_WIDTH];
    char buff[BUFF_SIZE];
    char input[BUFF_SIZE];

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
    win = newwin(WIN_HEIGHT, WIN_WIDTH, 0, 1);
    box(win, 0, 0);
    wrefresh(win);

    ReceiverData receiver_data = {
        .win = win,
        .receiver = receiver,
        .buff = buff
    };

    pthread_create(&t_receiver, NULL, receive_mq, (void *)&receiver_data);

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
    
    mq_close(receiver);

    exit(EXIT_SUCCESS);
}

void *receive_mq(void *param) {
    ReceiverData *data = (ReceiverData *)param;
    unsigned int prio = 1;
    static int row = 1;

    while(1) {
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