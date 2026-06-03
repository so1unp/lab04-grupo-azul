// send-mq.c 
// envia un mensaje a una cola de mensajes POSIX
// 
// Recibe como parámetros:
//    - El nombre de la cola a analizar
//    - El mensaje a enviar (un string)
//    - Prioridad del mensaje (default 0)
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

   char buff[256];
   int prio;

   // Controla parámetros de entrada
   if (argc<3)
   {   perror("uso: send-mq <nombre-cola> <mensaje (string)> <prio> \n"); exit(1); }

   sprintf (buff, "%s", argv[2]);
   
   prio = 0;
   if (argc == 4 ) 
	prio = atoi(argv[3]);

   /* Abre la cola de mensajes */
//   if ((cola = mq_open (argv[1],  O_RDWR | O_NONBLOCK )) == -1) 
   if ((cola = mq_open (argv[1],  O_RDWR )) == -1) 
    { perror("No se puede acceder a la cola de mensajes"); exit(1); }


   /* envia el mensaje */
   if (mq_send(cola, buff, strlen (buff),prio) == -1)
    { perror("Error al enviar el mensaje"); exit(1); }
    
   printf("Se envió el mensaje a la cola %s \n",argv[1]);

   mq_close(cola);

   exit(0);
}