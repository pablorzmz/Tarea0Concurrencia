#ifndef BUZON_H
#define BUZON_H

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <errno.h>
#include <iostream>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "Reserv.h"


#define KEY 0xB65728	// Valor de la llave del recurso

class Buzon {
public:
   Buzon();
   ~Buzon();
   int Send(Reserved content, int tipo);
   int Receive(Reserved &content, int tipo, int flag = 0 );   // len es el tamaño máximo que soporte la variable mensaje
  private:
   int id;		// Identificador del buzon
   // structura que identifica el tippo de mensaje y que es un mensaje como tal
   struct msgbuf
   {
       long mtype;
       Reserved messageContent;
   };
};

#endif // BUZON_H
