// crea-mq.c 
// Crea una cola de mensajes POSIX
// 
// Recibe como parámetros:
//    - El nombre de la cola a crear /M1
//    - La cantidad máxima de mensajes
//    - El tamaño máximo del mensaje
//
// Autor: CB - UNPSJB 
// 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include <mqueue.h>
#include <unistd.h>

#define QUEUE_PERMISSIONS 0666
#define MSG_BUFFER_SIZE MAX_MSG_SIZE + 10

int main (int argc, char *argv[ ])
{
   mqd_t cola;   		//descriptor de la cola
   struct mq_attr attr;   	//atributos de la cola
   int mqsize, msgsize;

   // Controla parámetros de entrada
   if (argc<4)
   {   perror("uso: crea-mq <nombre-cola> <cant-msgs> <tamaño-msgs>\n"); exit(1); }
   mqsize = atoi(argv[2]);
   msgsize = atoi(argv[3]);
  

   
   printf ("Va a crear la cola %s %i %i\n",argv[1],mqsize,msgsize);

   /* define los atributos de la cola a crear */
   attr.mq_flags = 0;			// Flags: 0 or O_NONBLOCK
   attr.mq_maxmsg = mqsize;
   attr.mq_msgsize = msgsize;
   attr.mq_curmsgs = 0;

   /* Crea la cola de mensajes */
   if ((cola = mq_open (argv[1],  O_CREAT | O_RDWR, QUEUE_PERMISSIONS, &attr)) == -1) 
   {   perror("No se puede crear la cola de mensajes"); exit(1); }

   // debug
   struct mq_attr check_attr;
   mq_getattr(cola, &check_attr);
   printf("Actual queue size: %ld\n", check_attr.mq_msgsize);
   
   printf("La cola fue creada con éxito\n");
   mq_close(cola);

   exit(0);
}