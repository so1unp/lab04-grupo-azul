#include <stdio.h>
#include <stdlib.h>
#include <ncurses.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <mqueue.h>
#include <signal.h>

#include "nave.h"
#include "config.h"

void mostrar_info(WINDOW *win, const Nave *nave);

volatile sig_atomic_t terminate_flag = 0;


void manejador_sigterm(int sig) {
    (void)sig;  // Evitar advertencia de variable no utilizada
    terminate_flag = 1;
}

/* Función para dibujar el mapa y la información de la nave */
static void dibujar(WINDOW *map_win, WINDOW *info_win, WINDOW *alert_win, EspacioCompartido *espacio_compartido, int my_id) {
    werase(map_win);
    werase(alert_win);
    box(map_win, 0, 0);
    box(alert_win, 0, 0);


    for (int y = 1; y < WIN_HEIGHT - 1; y++) {
        
        for (int x = 1; x < WIN_WIDTH - 1; x++) {

             if (y < WIN_ALERT_HEIGHT - 1) {
                mvwaddch(alert_win, y, x, (chtype)(unsigned char)espacio_compartido->mapAlert[y][x]);
            }
            mvwaddch(map_win, y, x, (chtype)(unsigned char)espacio_compartido->map[y][x]);
        }
    }

    wrefresh(map_win);
    wrefresh(alert_win);

    mostrar_info(info_win, &espacio_compartido->naves[my_id]);
}


int main() {
    /* if (argc < 2) {
        printf("Uso: %s <id_nave> (0-%d)\n", argv[0], MAX_NAVES - 1);
        return 1;
    } */

    int my_pid = getpid();
    /* if (my_id < 0 || my_id >= MAX_NAVES) {
        printf("ID de nave invalido (0-%d)\n", MAX_NAVES - 1);
        return 1;
    } */

    /* Creación e inicialización de la memoria compartida */
    int shm_fd;
    size_t total_shm_size = sizeof(EspacioCompartido);
    EspacioCompartido *espacio_compartido;

    shm_fd = shm_open(SHM_MAP_PATH, O_RDWR, 0);
    if (shm_fd < 0) {
        perror("Error al acceder a la memoria compartida del servidor");
        return 1;
    }

    // Mapeo memoria compartida
    espacio_compartido = mmap(NULL, total_shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (espacio_compartido == MAP_FAILED) {
        perror("Error en mmap");
        close(shm_fd);
        return 1;
    }

    bool createNave=0;
    for (int i = 0; i < MAX_NAVES   && createNave==0; i++) {
        
        if(espacio_compartido->naves[i].simbolo != 'N'){
            createNave=1;
            
        }

    }
    if(!createNave){
        printf("Naves maximas creadas\n");
        return 1;
    }

    struct sigaction sa;
    sa.sa_handler = manejador_sigterm; // Asignar la función
    sigemptyset(&sa.sa_mask);           // Bloquear otras señales mientras se ejecuta
    sa.sa_flags = 0;                   // Sin banderas especiales

    // Registrar el manejador para SIGTERM
    if (sigaction(SIGTERM, &sa, NULL) == -1) {
        perror("Error al registrar SIGTERM");
        return 1;
    }

    // Crear cola de mensajes
    mqd_t sender;
    if ((sender = mq_open(RECEIVER_MESSAGE_QUEUE, O_WRONLY)) == -1) {
        perror("Error al conectar con la cola del servidor");
        munmap(espacio_compartido, total_shm_size);
        close(shm_fd);
        return 1;
    }

    char msg[128];
    sprintf(msg, "I N %d", my_pid);
    mq_send(sender, msg, strlen(msg), 0);

    // Inicialización NCURSES
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);

    // Creación ventanas
    WINDOW *map_win = newwin(WIN_HEIGHT, WIN_WIDTH, 0, 0);
    WINDOW *alert_win = newwin(WIN_ALERT_HEIGHT, WIN_WIDTH, WIN_HEIGHT, 0);
    WINDOW *info_win = newwin(14, 35, 0, WIN_WIDTH + 2);
    wtimeout(map_win, 100);

    // variable para almacenar el ID de la nave
    int my_id = -1;
    for(int i = 0; i < MAX_NAVES; i++) {
        if (espacio_compartido->naves[i].id == my_pid) {
            my_id = i;
            break;
        }
    }

    // Loop principal
    int salir = 0;
    while (!salir && !terminate_flag) {
        dibujar(map_win, info_win, alert_win, espacio_compartido, my_id);

        int tecla = wgetch(map_win);
        int dx = 0, dy = 0;
        int compra=-1;

        switch (tecla) {
            case 'w': dy = -1; break;
            case 's': dy = 1; break;
            case 'a': dx = -1; break;
            case 'd': dx = 1; break;
            case 'q': salir = 1; break;
            case 'Q': salir = 1; break;
            //case 'v': compra = 0; break;
            case 'e': compra = 1; break;
            case '1': compra = 2; break;
            case '2': compra = 3; break;
            case '3': compra = 4; break;
            case '4': compra = 5; break;
            default: break;
        }

        if ((dx != 0 || dy != 0) && espacio_compartido->naves[my_id].activa) {
            sprintf(msg, "M N %d %d %d", my_pid, dx, dy);
            mq_send(sender, msg, strlen(msg), 0);
        }
        if (compra != -1 && espacio_compartido->naves[my_id].activa) {
            sprintf(msg, "C N %d %d", my_pid, compra);
            mq_send(sender, msg, strlen(msg), 0);
        }
        
    }

    werase(map_win);
    werase(info_win);
    werase(alert_win);
    endwin();

    mq_close(sender);
    munmap(espacio_compartido, total_shm_size);
    close(shm_fd);

    if(terminate_flag) {
        printf("Nave %d terminada por el servidor\n", my_pid);
    } else {
        printf("Nave %d terminada por usuario\n", my_pid);
    }

    exit(EXIT_SUCCESS);
}

/* Función para mostrar información de la nave */
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
    mvwprintw(win, 5, 2, "Escudo:      %d", nave->escudo);

    if (nave->activa) {
        mvwprintw(win, 6, 2, "Estado:      ACTIVA");
    } else {
        mvwprintw(win, 6, 2, "Estado:      DESACTIVADA");
    }

    mvwprintw(win, 8, 2, "Cargamento:");
    mvwprintw(win, 9, 4, "Deuterio:   %d", nave->cargamento[IDX_DEUTERIO]);
    mvwprintw(win, 10, 4, "Mutexio:    %d", nave->cargamento[IDX_MUTEXIO]);
    mvwprintw(win, 11, 4, "Semaforita: %d", nave->cargamento[IDX_SEMAFORITA]);
    mvwprintw(win, 12, 4, "Kernelio:   %d", nave->cargamento[IDX_KERNELIO]);

    wrefresh(win);
}
