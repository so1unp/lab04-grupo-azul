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
#include "estacion.h"

#define BUFF_SIZE 1024
#define ASTEROID_SYMBOL '*'
#define LOG_FILE "log.txt"

Asteroide asteroides[NUM_ASTEROIDS];
EspacioCompartido *espacio_compartido;

// Mutex global para el log.txt
pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

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
void *loop_estacion(void *param);

void place_asteroids(char map[][WIN_WIDTH]);
void manejo_nave(char tipo, int id, int arg1, int arg2);
void loop_compras(int id, int arg1);
void compra (int idNave, int idEstacion, int compra);
void vender(int idNave, int idEstacion);
void compraCombustible(int idNave, int idEstacion);
void compraOxigeno(int idNave, int idEstacion);
void compraArmadura(int idNave, int idEstacion);
void compraSuperArmadura(int idNave, int idEstacion);
void compraCondimentoPizza(int idEstacion);
//void compras(int idNave, int idEstacion, int compra);

static void log_transaccion_asteroide(int nave_id, int ast_id, int deut, int mut, int sem, int ker);
static void log_transaccion_nave(int nave_id, int muerta_id, int deut, int mut, int sem, int ker, int comb, int ox);

/* Busca el índice del asteroide en la posición (nx, ny). Retorna -1 si no lo encuentra */
static int buscar_asteroide(int nx, int ny) {
    for (int i = 0; i < NUM_ASTEROIDS; i++) {
        if (asteroides[i].active &&
            asteroides[i].col == nx &&
            asteroides[i].row == ny)
            return i;
    }
    return -1;
}

/* Busca el índice de la nave en la posición (nx, ny). Retorna -1 si no lo encuentra */
static int buscar_nave(int nx, int ny) {
    for (int i = 0; i < MAX_NAVES; i++) {
        if (espacio_compartido->naves[i].x == nx &&
            espacio_compartido->naves[i].y == ny)
            return i;
    }
    return -1;
}

/* Extrae los minerales del asteroide y los guarda en el cargamento de la nave.
   Lock solo sobre el asteroide tocado, otros asteroides siguen libres. */
void extraer_asteroide(int nave_id, int nx, int ny) {
    if (nave_id < 0 || nave_id >= MAX_NAVES)                              return;
    if (!espacio_compartido->naves[nave_id].activa)                        return;
    if (espacio_compartido->naves[nave_id].combustible < COSTO_EXTRACCION) return;

    int idx = buscar_asteroide(nx, ny);
    if (idx == -1) return;

    Asteroide *a = &asteroides[idx];

    pthread_mutex_lock(&a->mutex);

    if (!a->active) {   /* otra nave llegó primero */
        pthread_mutex_unlock(&a->mutex);
        return;
    }

    int d = a->deuterio;
    int m = a->mutexio;
    int s = a->semaforita;
    int k = a->kernelio;

    espacio_compartido->naves[nave_id].combustible -= COSTO_EXTRACCION;

    espacio_compartido->naves[nave_id].cargamento[IDX_DEUTERIO]   += d;
    espacio_compartido->naves[nave_id].cargamento[IDX_MUTEXIO]    += m;
    espacio_compartido->naves[nave_id].cargamento[IDX_SEMAFORITA] += s;
    espacio_compartido->naves[nave_id].cargamento[IDX_KERNELIO]   += k;

    a->deuterio   = 0;
    a->mutexio    = 0;
    a->semaforita = 0;
    a->kernelio   = 0;
    a->active     = 0;

    pthread_mutex_unlock(&a->mutex);

    log_transaccion_asteroide(nave_id, idx, d, m, s, k);
}

void crearEstacion(char tipo, int id) {
    if (id < 0 || id >= NUM_STATIONS) return;
     
    int row = 1 + rand() % (WIN_HEIGHT - 2);
    int col = 1 + rand() % (WIN_WIDTH  - 2);

    while(espacio_compartido->map[row][col] != ' ' || (row == WIN_HEIGHT / 2 && col == WIN_WIDTH / 2)){
       row = 1 + rand() % (WIN_HEIGHT - 2);
       col = 1 + rand() % (WIN_WIDTH  - 2);
    }

    if (tipo == 'I') {
        if (!espacio_compartido->estaciones[id].activa) {
            espacio_compartido->estaciones[id].id          = id;
            espacio_compartido->estaciones[id].x           = col;
            espacio_compartido->estaciones[id].y           = row;
            espacio_compartido->estaciones[id].combustible = COMBUSTIBLE_INICIAL;
            espacio_compartido->estaciones[id].oxigeno     = OXIGENO_INICIAL_ESTACION;
            espacio_compartido->estaciones[id].billetera   = BILLETERA_INICIAL;
            espacio_compartido->estaciones[id].activa      = 1;
            espacio_compartido->estaciones[id].hangar      = HANGAR;
            espacio_compartido->estaciones[id].simbolo     = 'E';
            espacio_compartido->estaciones[id].cargamento[IDX_DEUTERIO]   = 0;
            espacio_compartido->estaciones[id].cargamento[IDX_MUTEXIO]    = 0;
            espacio_compartido->estaciones[id].cargamento[IDX_SEMAFORITA] = 0;
            espacio_compartido->estaciones[id].cargamento[IDX_KERNELIO]   = 0;
            
            pthread_mutex_init(&espacio_compartido->estaciones[id].mutex, NULL); /* mutex independiente por estacion */

            espacio_compartido->map[espacio_compartido->estaciones[id].y][espacio_compartido->estaciones[id].x] = 'E';
        }
    }
}

/* Extrae los recursos de una nave muerta.
   Lock solo sobre la nave tocada */
void extraer_nave_muerta(int nave_id, int nx, int ny) {
    if (nave_id < 0 || nave_id >= MAX_NAVES)                              return;
    if (!espacio_compartido->naves[nave_id].activa)                        return;
    if (espacio_compartido->naves[nave_id].combustible < COSTO_EXTRACCION) return;

    int idx = buscar_nave(nx, ny);
    if (idx == -1) return;

    Nave *n = &espacio_compartido->naves[idx];

    pthread_mutex_lock(&n->mutex);

    int d = n->cargamento[IDX_DEUTERIO];
    int m = n->cargamento[IDX_MUTEXIO];
    int s = n->cargamento[IDX_SEMAFORITA];
    int k = n->cargamento[IDX_KERNELIO];
    int c_extra = n->combustible;
    int o_extra = n->oxigeno;

    espacio_compartido->naves[nave_id].combustible -= COSTO_EXTRACCION;

    espacio_compartido->naves[nave_id].combustible += c_extra;
    espacio_compartido->naves[nave_id].oxigeno += o_extra;
    espacio_compartido->naves[nave_id].cargamento[IDX_DEUTERIO]   += d;
    espacio_compartido->naves[nave_id].cargamento[IDX_MUTEXIO]    += m;
    espacio_compartido->naves[nave_id].cargamento[IDX_SEMAFORITA] += s;
    espacio_compartido->naves[nave_id].cargamento[IDX_KERNELIO]   += k;

    n->combustible = 0;
    n->oxigeno = 0;
    n->cargamento[IDX_DEUTERIO]   = 0;
    n->cargamento[IDX_MUTEXIO]    = 0;
    n->cargamento[IDX_SEMAFORITA] = 0;
    n->cargamento[IDX_KERNELIO]   = 0;

    pthread_mutex_unlock(&n->mutex);

    log_transaccion_nave(nave_id, idx, d, m, s, k, c_extra, o_extra);
}

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    setlocale(LC_ALL, "");
    srand((unsigned int)time(NULL));

    WINDOW *win;
    pthread_t t_receiver, t_printer, t_game, t_estacion;
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

    /* Inicializa asteroides y sus mutexes individuales */
    place_asteroids(espacio_compartido->map);

    mqd_t receiver;
    struct mq_attr attr;
    attr.mq_flags   = 0;
    attr.mq_maxmsg  = 10;
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
        .win      = win,
        .receiver = receiver,
        .buff     = buff
    };

    MapData m_data = {
        .win = win
    };

    pthread_create(&t_receiver, NULL, receive_mq, (void *)&r_data);
    pthread_create(&t_printer,  NULL, print_map,  (void *)&m_data);
    pthread_create(&t_game,     NULL, loop_juego, NULL);
    pthread_create(&t_estacion, NULL, loop_estacion, NULL);
    //pthread_create(&t_compras,  NULL, loop_compras, NULL);

    while (1) {
        int ch = wgetch(win);
        if (ch == 'q' || ch == 'Q') break;
        usleep(50000);
    }

    werase(win);
    endwin();

    pthread_cancel(t_receiver);
    pthread_cancel(t_printer);
    pthread_cancel(t_game);
    pthread_cancel(t_estacion);

    pthread_join(t_receiver, NULL);
    pthread_join(t_printer,  NULL);
    pthread_join(t_game,     NULL);
    pthread_join(t_estacion, NULL);

    mq_close(receiver);
    mq_unlink(RECEIVER_MESSAGE_QUEUE);

    /* Destruir mutexes */
    for (int i = 0; i < NUM_ASTEROIDS; i++) {
        pthread_mutex_destroy(&asteroides[i].mutex);
    }
    pthread_mutex_destroy(&log_mutex);

    munmap(espacio_compartido, total_shm_size);
    close(shm_fd);
    shm_unlink(SHM_MAP_PATH);

    exit(EXIT_SUCCESS);
}

void *receive_mq(void *param) {
    ReceiverData *data = (ReceiverData *)param;
    unsigned int prio = 1;

    while (1) {
        memset(data->buff, 0, BUFF_SIZE);

        if (mq_receive(data->receiver, data->buff, BUFF_SIZE, &prio) == -1) {
            perror("Error al recibir el mensaje");
            return NULL;
        }

        char tipo, entidad;
        int id = 0, arg1 = 0, arg2 = 0;

        if (sscanf(data->buff, "%c %c %d %d %d", &tipo, &entidad, &id, &arg1, &arg2) >= 2) {
            if (entidad == 'N' && (tipo == 'M' || tipo == 'I')) {
                manejo_nave(tipo, id, arg1, arg2);
            }

            if (entidad == 'N' && tipo == 'C') {
                loop_compras(id, arg1);
            }

            if (entidad == 'E') {
                crearEstacion(tipo, id);
            }
        }
    }
}

void *print_map(void *param) {
    MapData *data = (MapData *)param;

    while (1) {
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
   
}

void *loop_juego(void *param) {
    (void)param;
    while (1) {
        sleep(1);
        for (int i = 0; i < NUM_STATIONS; i++) {
         if (espacio_compartido->estaciones[i].activa) {
                pthread_mutex_lock(&espacio_compartido->estaciones[i].mutex);
                espacio_compartido->estaciones[i].combustible -= COSTO_MOVIMIENTO;
                
                pthread_mutex_unlock(&espacio_compartido->estaciones[i].mutex);
            }
            if (espacio_compartido->estaciones[i].combustible == 20) {
                int start_y = (WIN_HEIGHT - WIN_ALERT_HEIGHT) / 2;
                int start_x = (WIN_WIDTH - WIN_ALERT_WIDTH) / 2;

                WINDOW *modal = newwin(WIN_ALERT_HEIGHT, WIN_ALERT_WIDTH, start_y, start_x);
                box(modal, 0, 0);
                mvwprintw(modal, 1, 2, "ADVERTENCIA");
                mvwprintw(modal, 3, 2, "Estacion %d casi sin combustible!", espacio_compartido->estaciones[i].id);
                
                wrefresh(modal);
                sleep(2);

                // Limpiar
                delwin(modal);
            }
            if (espacio_compartido->estaciones[i].combustible <= 0) {
                espacio_compartido->estaciones[i].activa = 0;
            }
        
        
        }

        for (int i = 0; i < MAX_NAVES; i++) {
            if (espacio_compartido->naves[i].activa) {
                if (espacio_compartido->naves[i].oxigeno > 0) {
                    espacio_compartido->naves[i].oxigeno --;
                    if (espacio_compartido->naves[i].oxigeno == 0) {
                        espacio_compartido->naves[i].activa = 0;
                        espacio_compartido->map[espacio_compartido->naves[i].y][espacio_compartido->naves[i].x] = 'X';
                    }
                }
            }
        }
    }
   
}

void place_asteroids(char map[][WIN_WIDTH]) {
    int placed = 0;
    while (placed < NUM_ASTEROIDS) {
        int row = 1 + rand() % (WIN_HEIGHT - 2);
        int col = 1 + rand() % (WIN_WIDTH  - 2);

        if (map[row][col] != ' ' || (row == WIN_HEIGHT / 2 && col == WIN_WIDTH / 2)) continue;
       
        Asteroide *asteroide = &asteroides[placed];
        asteroide->row        = row;
        asteroide->col        = col;
        asteroide->active     = 1;
        asteroide->deuterio   = 20 + rand() % 31;
        asteroide->mutexio    = (rand() % 2) ? (5 + rand() % 6) : 0;
        asteroide->semaforita = (rand() % 2) ? (3 + rand() % 6) : 0;
        asteroide->kernelio   = (rand() % 2) ? (1 + rand() % 3) : 0;
        asteroide->oxigeno    = (rand() % 5 == 0) ? (1 + rand() % 5) : 0;
        asteroide->aleacion   = (rand() % 5 == 0) ? (1 + rand() % 3) : 0;
        asteroide->condimento = (rand() % 5 == 0) ? (1 + rand() % 4) : 0;

        pthread_mutex_init(&asteroide->mutex, NULL); /* mutex independiente por asteroide */

        map[row][col] = ASTEROID_SYMBOL;
        placed++;
    }
}

void loop_compras(int id, int compraNum) {
    if (id < 0 || id >= MAX_NAVES) return;
    
    if (espacio_compartido->naves[id].activa) {
        for (int j = 0; j < NUM_STATIONS; j++) {
            if (espacio_compartido->estaciones[j].activa) {
                int dx = abs(espacio_compartido->naves[id].x - espacio_compartido->estaciones[j].x);
                int dy = abs(espacio_compartido->naves[id].y - espacio_compartido->estaciones[j].y);
                if (dx <= 1 && dy <= 1) {
                    // La nave está adyacente a la estación j
                    compra(id, j, compraNum);
                }
            }
        }
    }
}

void compra (int idNave, int idEstacion, int compraNum) {
    pthread_mutex_lock(&espacio_compartido->estaciones[idEstacion].mutex);
    pthread_mutex_lock(&espacio_compartido->naves[idNave].mutex);
    
    switch (compraNum) {

        case 0: vender(idNave, idEstacion); break;
        case 1: compraCombustible(idNave, idEstacion); break;
        case 2: compraOxigeno(idNave, idEstacion); break;
        case 3: compraArmadura(idNave, idEstacion); break;
        case 4: compraSuperArmadura(idNave, idEstacion); break;
        case 5: compraCondimentoPizza(idEstacion); break;
        default: break;
    }
    
    pthread_mutex_unlock(&espacio_compartido->naves[idNave].mutex);
    pthread_mutex_unlock(&espacio_compartido->estaciones[idEstacion].mutex);
}

void vender(int idNave, int idEstacion) {
    if (espacio_compartido->naves[idNave].cargamento[IDX_DEUTERIO] > 0) {
        espacio_compartido->estaciones[idEstacion].cargamento[IDX_DEUTERIO] += espacio_compartido->naves[idNave].cargamento[IDX_DEUTERIO];
        espacio_compartido->naves[idNave].cargamento[IDX_DEUTERIO] = 0;
    }
    if (espacio_compartido->naves[idNave].cargamento[IDX_MUTEXIO] > 0) {
        espacio_compartido->estaciones[idEstacion].cargamento[IDX_MUTEXIO] += espacio_compartido->naves[idNave].cargamento[IDX_MUTEXIO];
        espacio_compartido->naves[idNave].cargamento[IDX_MUTEXIO] = 0;
    }
    if (espacio_compartido->naves[idNave].cargamento[IDX_SEMAFORITA] > 0) {
        espacio_compartido->estaciones[idEstacion].cargamento[IDX_SEMAFORITA] += espacio_compartido->naves[idNave].cargamento[IDX_SEMAFORITA];
        espacio_compartido->naves[idNave].cargamento[IDX_SEMAFORITA] = 0;
    }
    if (espacio_compartido->naves[idNave].cargamento[IDX_KERNELIO] > 0) {
        espacio_compartido->estaciones[idEstacion].cargamento[IDX_KERNELIO] += espacio_compartido->naves[idNave].cargamento[IDX_KERNELIO];
        espacio_compartido->naves[idNave].cargamento[IDX_KERNELIO] = 0;
    }
}

void compraCombustible(int idNave, int idEstacion) {
    if (espacio_compartido->estaciones[idEstacion].combustible > 20 && espacio_compartido->naves[idNave].combustible < COMBUSTIBLE_INICIAL) {
        espacio_compartido->estaciones[idEstacion].combustible --;
        espacio_compartido->naves[idNave].combustible ++;
    }
}

void compraOxigeno(int idNave, int idEstacion) {
    if (espacio_compartido->estaciones[idEstacion].billetera >= price_oxigeno && espacio_compartido->naves[idNave].oxigeno < OXIGENO_INICIAL) {
        espacio_compartido->estaciones[idEstacion].billetera -= price_oxigeno;
        espacio_compartido->naves[idNave].oxigeno ++;
    }
}

void compraArmadura(int idNave, int idEstacion) {
    if (espacio_compartido->estaciones[idEstacion].billetera >= price_reparar && espacio_compartido->naves[idNave].escudo < ESCUDO_INICIAL) {
        espacio_compartido->estaciones[idEstacion].billetera -= price_reparar;
        espacio_compartido->naves[idNave].escudo ++;
    }
}

void compraSuperArmadura(int idNave, int idEstacion) {
    if (espacio_compartido->estaciones[idEstacion].billetera >= price_super_armadura) {
        espacio_compartido->estaciones[idEstacion].billetera -= price_super_armadura;
        espacio_compartido->naves[idNave].escudo = SUPER_ARMADURA;
    }
}

void compraCondimentoPizza(int idEstacion) {
    if (espacio_compartido->estaciones[idEstacion].billetera >= price_condimento) {
        espacio_compartido->estaciones[idEstacion].billetera -= price_condimento;
        place_asteroids(espacio_compartido->map);
    }
}

void *loop_estacion(void *param) {
    (void)param;
    while (1) {
        sleep(1);
        for (int i = 0; i < NUM_STATIONS; i++) {
            if (espacio_compartido->estaciones[i].activa) {
               
                pthread_mutex_lock(&espacio_compartido->estaciones[i].mutex);

                espacio_compartido->estaciones[i].combustible += espacio_compartido->estaciones[i].cargamento[IDX_DEUTERIO] ;
                espacio_compartido->estaciones[i].billetera += espacio_compartido->estaciones[i].cargamento[IDX_MUTEXIO]*price_mutexio ;
                espacio_compartido->estaciones[i].billetera += espacio_compartido->estaciones[i].cargamento[IDX_SEMAFORITA]*price_semaforita ;
                espacio_compartido->estaciones[i].billetera += espacio_compartido->estaciones[i].cargamento[IDX_KERNELIO]*price_kernelio ;

                espacio_compartido->estaciones[i].cargamento[IDX_DEUTERIO]=0;
                espacio_compartido->estaciones[i].cargamento[IDX_MUTEXIO]=0;
                espacio_compartido->estaciones[i].cargamento[IDX_SEMAFORITA]=0;
                espacio_compartido->estaciones[i].cargamento[IDX_KERNELIO]=0;
                
                pthread_mutex_unlock(&espacio_compartido->estaciones[i].mutex);
            }
        }
    }
    
}

void manejo_nave(char tipo, int id, int arg1, int arg2) {
    if (id < 0 || id >= MAX_NAVES) return;

    if (tipo == 'I') {
        if (!espacio_compartido->naves[id].activa) {
            espacio_compartido->naves[id].id          = id;
            espacio_compartido->naves[id].x           = WIN_WIDTH  / 2;
            espacio_compartido->naves[id].y           = WIN_HEIGHT / 2;
            espacio_compartido->naves[id].combustible = COMBUSTIBLE_INICIAL;
            espacio_compartido->naves[id].oxigeno     = OXIGENO_INICIAL;
            espacio_compartido->naves[id].escudo      = ESCUDO_INICIAL;
            espacio_compartido->naves[id].activa      = 1;
            espacio_compartido->naves[id].simbolo     = 'N';
            pthread_mutex_init(&espacio_compartido->naves[id].mutex, NULL); /* mutex independiente por nave */

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
                        espacio_compartido->map[ny][nx] = 'X';
                    }
                }
            }
            else if (espacio_compartido->map[ny][nx] == ASTEROID_SYMBOL) {
                /* La nave chocó con un asteroide, extrae sus minerales */
                extraer_asteroide(id, nx, ny);
            }
            else if (espacio_compartido->map[ny][nx] == 'X') {
                /* La nave chocó con una nave muerta, extrae sus minerales */
                extraer_nave_muerta(id, nx, ny);
            }
        }
    }
}

static void log_transaccion_asteroide(int nave_id, int ast_id, int deut, int mut, int sem, int ker) {
    pthread_mutex_lock(&log_mutex);
    FILE *f = fopen(LOG_FILE, "a");
    if (f != NULL) {
        time_t ahora = time(NULL);
        struct tm *t_info = localtime(&ahora);
        char timestamp[20];
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", t_info);

        fprintf(f, "[%s] Extraccion Asteroide: Nave %d recolecto de Asteroide %d: Deuterio: %d, Mutexio: %d, Semaforita: %d, Kernelio: %d\n",
                timestamp, nave_id, ast_id, deut, mut, sem, ker);
        fclose(f);
    }
    pthread_mutex_unlock(&log_mutex);
}

static void log_transaccion_nave(int nave_id, int muerta_id, int deut, int mut, int sem, int ker, int comb, int ox) {
    pthread_mutex_lock(&log_mutex);
    FILE *f = fopen(LOG_FILE, "a");
    if (f != NULL) {
        time_t ahora = time(NULL);
        struct tm *t_info = localtime(&ahora);
        char timestamp[20];
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", t_info);

        fprintf(f, "[%s] Extraccion Nave Muerta: Nave %d saqueo a Nave Muerta %d: Deuterio: %d, Mutexio: %d, Semaforita: %d, Kernelio: %d, Combustible: %d, Oxigeno: %d\n",
                timestamp, nave_id, muerta_id, deut, mut, sem, ker, comb, ox);
        fclose(f);
    }
    pthread_mutex_unlock(&log_mutex);
}