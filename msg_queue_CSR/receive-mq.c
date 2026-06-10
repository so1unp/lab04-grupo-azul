// receive-mq.c 
// recibe un mensaje de una cola de mensajes POSIX
// 
// Recibe como parámetros:
//    - El nombre de la cola a analizar
//    - Prioridad del mensaje a recibir (default 0)
//
// Autor: CB - UNPSJB 
// 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>

#define QUEUE_PERMISSIONS 0644

int main (int argc, char *argv[ ])
{
   mqd_t cola;   		//descriptor de la cola
   struct mq_attr attr;

   char buff[1024+10];
   unsigned int prio;

   // Controla parámetros de entrada
   if (argc<2)
   {   perror("uso: send-mq <nombre-cola>  <prio> \n"); exit(1); }

   prio = 0;
   if (argc ==3 ) 
	prio = atoi(argv[2]);

   /* Abre la cola de mensajes */
   if ((cola = mq_open (argv[1],  O_RDWR)) == -1) 
    { perror("No se puede acceder a la cola de mensajes"); exit(1); }


   /* recibe el mensaje */
   if (mq_receive(cola, buff, sizeof(buff)+10,&prio) == -1)
    { perror("Error al recibir el mensaje"); exit(1); }
    
   printf("Se recibió el mensaje [%s] \n",buff);

   mq_close(cola);

   exit(0);
}