#include <stdio.h>
#include <stdlib.h>
#include "nave.h" /*estructura de la nave y prototipos de funciones, se puede poner un struct sin necesidad de hacer nave.h (si quieren hacerlo de esa manera lo puedes modificar nomas, 2 archivos en uno)
*/
// valores iniciales para una nave recien creada (constantes)
#define COMBUSTIBLE_INICIAL  100
#define OXIGENO_INICIAL      100
#define COSTO_MOVIMIENTO       1   /*cada movimiento del teclado gasta combustible */

Nave *nave_crear(int id, int x_inicial, int y_inicial) //funcion para crear una nave, recibe un id y una posicion inicial (x,y)
{
    Nave *nave = malloc(sizeof(Nave)); //reservo espacio en la RAM para guardar datos de la nave
    if (nave == NULL) {                //si no se pudo reservar espacio en la RAM, se muestra un mensaje de error y se retorna NULL
        perror("malloc nave");         //muestra mensaje de error
        return NULL;                   //no se pudo crear la nave por lo tanto retorna NULL
    }

    nave->id          = id;                     //asigno el id a la nave
    nave->x           = x_inicial;              //inicializo la posicion de la nave con los valores recibidos por parametro
    nave->y           = y_inicial;              //inicializo la posicion de la nave con los valores recibidos por parametro
    nave->combustible = COMBUSTIBLE_INICIAL;    //inicializo el combustible en 100
    nave->oxigeno     = OXIGENO_INICIAL;        //inicializo el nivel de oxigeno en 100
    nave->activa      = 1;                      //1= vivo , 0= muerto (game over) 
    nave->simbolo     = 'N';  //es lo que se representara en el espacio, osea nave es un N :v

    /* Cargamento vacio */
    for (int i = 0; i < NUM_RECURSOS; i++) {        //inicializo el cargamento de recursos en 0, (no hay nada por que la nave recien se creo)
        nave->cargamento[i] = 0;
    }

    return nave;
}
//libera memoria

void nave_destruir(Nave *nave)
{
    if (nave != NULL) {
        free(nave);
    }
}

// imprime los datos en pantalla de la nave
void nave_mostrar_info(const Nave *nave) {
    if (nave == NULL){ //si la nave no fue creada entonces 
    return;
    }
    printf("  Nave %d\n", nave->id);
    printf("  Posicion:    (%d, %d)\n", nave->x, nave->y);
    printf("  Combustible: %d\n", nave->combustible);
    printf("  Oxigeno:     %d\n", nave->oxigeno);
    if (nave->activa) {     // 1 = activa, 0 = desactivada (game over)
    printf("  Estado:      ACTIVA\n");
    } else {
    printf("  Estado:      DESACTIVADA\n");
    }
    printf("  Cargamento:\n");                                          //de aca para abajo falta 
    printf("    Deuterio:   %d\n", nave->cargamento[IDX_DEUTERIO]);                 
    printf("    Mutexio:    %d\n", nave->cargamento[IDX_MUTEXIO]);
    printf("    Semaforita: %d\n", nave->cargamento[IDX_SEMAFORITA]);
    printf("    Kernelio:   %d\n", nave->cargamento[IDX_KERNELIO]);
}

//mueve la nave
int nave_mover(Nave *nave, int dx, int dy, int ancho_mapa, int alto_mapa){
    if (nave == NULL || !nave->activa) return 0; 

    //Calcular nueva posicion tentativa
    int nueva_x = nave->x + dx;
    int nueva_y = nave->y + dy;

    //Verificar que no se salga del mapa
    if (nueva_x < 0 || nueva_x >= ancho_mapa){ 
    return 0;
    }   
    if (nueva_y < 0 || nueva_y >= alto_mapa){ 
        return 0;
    }
    //si no tiene combustible,  no se mueve mas
    if (!nave_consumir_combustible(nave, COSTO_MOVIMIENTO)) {
        return 0;
    }
    //sino
    nave->x = nueva_x;
    nave->y = nueva_y;
    return 1;
}

int nave_consumir_combustible(Nave *nave, int cantidad)
{
    if (nave == NULL || !nave->activa) {return 0;
    }
    if (nave->combustible < cantidad) {
        nave->combustible = 0;
        nave->activa = 0;  /* Sin combustible => desactivada */
        return 0;
    }

    nave->combustible -= cantidad;
    if (nave->combustible == 0) {
        nave->activa = 0;
    }
    return 1;
}

// issues #17... falta de oxigeno
int nave_consumir_oxigeno(Nave *nave, int cantidad)
{
    if (nave == NULL || !nave->activa) {
        return 0;
    }
    if (nave->oxigeno < cantidad) {
        nave->oxigeno = 0;
        nave->activa = 0;  /* Sin oxigeno => desactivada */
        return 0;
    }

    nave->oxigeno -= cantidad;
    if (nave->oxigeno == 0) {
        nave->activa = 0;
    }
    return 1;
}
