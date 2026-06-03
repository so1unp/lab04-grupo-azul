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
#define BUF_SIZE 256

int main(int argc, char *argv[])
{
    srand(time(NULL));

    WINDOW *win;
    char map[WIN_HEIGHT][WIN_WIDTH];
    char buf[BUF_SIZE];

    /* Inicializa Ncurses */
    initscr();

    clear();
    refresh();

    win = newwin(WIN_HEIGHT, WIN_WIDTH, 0, 1);
    box(win, 0, 0);
    wrefresh(win);

    int row = 1;
    while(strcmp(buf, "FIN") != 0) {
        wmove(win, row, 2);
        wgetstr(win, buf);

        row++;
        if(row > WIN_HEIGHT - 2) {
            row = 1;
        }
    }

    // Termina la ejecución del programa.
    werase(win);
    endwin();
    
    exit(EXIT_SUCCESS);
}
