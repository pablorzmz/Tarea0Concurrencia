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
   int Send(Reserve content, int tipo);
   int Receive(Reserve &content, int tipo );   // len es el tamaño máximo que soporte la variable mensaje
  private:
   int id;		// Identificador del buzon
   struct msgbuf
   {
       long mtype;
       Reserve messageContent;
   };
};

#endif // BUZON_H
