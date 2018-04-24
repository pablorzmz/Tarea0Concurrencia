#ifndef ADMINJUST_H
#define ADMINJUST_H

#include "Buzon.h"
#include "Justify.h"
#include "Semaforo.h"
#include "Reserv.h"
#include <string.h>
#include <sys/ipc.h>
#include <sys/wait.h>
#include <unistd.h>

class AdminJust
{
public:
    AdminJust();
    ~AdminJust();
    int run(std::string listOfFiles[], int numberOfFiles, short identationSize);
private:
    Buzon messageControl;
    Semaforo semProcesses;


};

#endif // ADMINJUST_H
