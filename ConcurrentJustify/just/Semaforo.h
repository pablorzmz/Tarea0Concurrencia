#ifndef SEMAFORO_H
#define SEMAFORO_H

#define KEY2 0x5728	// Valor de la llave del recurso

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <errno.h>
#include <iostream>
#include <unistd.h>

class Semaforo {
  public:
    Semaforo( int ValorInicial = 0 );
   ~Semaforo();
   int Signal(const int &increment = 1 );	// Puede llamarse V
   int Wait(const int &decrement = -1);	// Puede llamarse P
  private:
   int id;		// Identificador del semaforo
};

#endif // SEMAFORO_H
