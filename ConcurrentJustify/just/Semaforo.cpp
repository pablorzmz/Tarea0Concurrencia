#include "Semaforo.h"

Semaforo::Semaforo(int ValorInicial, int KEY2)
    :id(0)
{
    // se guarda el id del semaforo
    id = semget(KEY2,1,IPC_CREAT|0600);

    // verificamos que la creación del semáforo sea correcta
    if (-1 == id )
    {
        perror("Error en el constructor del semáforo");
        _exit(1);
    }

    // estructura para los atributos de control
    union semun {
        int val;
        struct semid_ds *buf;
        unsigned short  *array;
        struct seminfo  *__buf;
    };

    // se inicializa el valor del semáforo
    semun atributos;
    atributos.val = ValorInicial;

    int result = semctl( id, 0, SETVAL, atributos);

    // se comprueba que la operación sea la correcta
    if (-1 == result )
    {
        perror("Error en el constructor del semáforo");
        _exit(1);
    }
}

int Semaforo::Signal(const int &increment)
{
    sembuf args;

    args.sem_num  = 0; // indica que es el primer semáforo
    args.sem_op = increment; // para incrementar en 1 el semáforo
    args.sem_flg = 0; // sin valor especifico para las banderas

    // realizamos la operación, solo una para el semáforo
    int result = semop( id, &args , 1);

    // validamos
    if( -1 == result )
    {
        perror("Error al hacer signal al semáforo");
        _exit(1);
    }

    return result;
}

int Semaforo::Wait(const int &decrement)
{
    sembuf args;

    args.sem_num  = 0; // indica que es el primer semáforo
    args.sem_op = decrement; // para incrementar en 1 el semáforo
    args.sem_flg = 0; // sin valor especifico para las banderas

    // realizamos la operación, solo una para el semáforo
    int result = semop( id, &args , 1);

    // validamos
    if( -1 == result )
    {
        perror("Error al hacer wait al semáforo");
        _exit(1);
    }
    return result;
}
Semaforo::~Semaforo()
{
    int result = semctl(id,0,IPC_RMID);
    if ( -1 == result )
    {
        perror("SEMAFORO:: Error en el destructor ");
        _exit(0);
    }else
    {
        std::cout<<"SEMAFORO::Semaforo destruido satisfactoriamente"<<std::endl;

    }
}
