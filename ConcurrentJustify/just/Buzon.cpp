#include "Buzon.h"

Buzon::Buzon()
{
    this->id = msgget(KEY,IPC_CREAT|0600); // se obtiene el buzón del ábol de procesos actual
    if ( -1 == id ) // si no hubo un error, la ejecución continúa normalmente
    {
        perror("Error a la hora de contruir el buzón");
        _exit(1);
    }
}


Buzon::~Buzon()
{
    // se ejecuta la llamada del sistema para que cuando se destruya el buzón,
    // se librere el buzón otrogado
    int result = msgctl( id, IPC_RMID, NULL);
    if ( -1== result )
    {
        perror("BUZON:: error a la hora de destruir el buzon");
        _exit(0); // sólo un proceso asociado al buzón lo destruye
    }else
    {
        std::cout<<"BUZON:: Buzón destruido"<<std::endl;
    }
}

int Buzon::Send( Reserve content, int tipo )
{    
    msgbuf mensajeNuevo; // se crea un nuevo objeto del tipo msgbuf, que estructura un mensaje
    mensajeNuevo.mtype = tipo; // la prioridad de los mensajes, en este buzón, siempre va a ser de 1
    mensajeNuevo.messageContent = content; // se copia el contenido de la variable a la estructura del mensaje
    int result = msgsnd( id, (void*) &mensajeNuevo, sizeof(mensajeNuevo),IPC_NOWAIT);

    if ( -1 == result ) // si hubo error al envío, se finaliza la ejecución.
    {
        perror("Error al enviar el mensaje");
        _exit(1);
    }
    else
    {
        return 0;
    }
}


int Buzon::Receive(Reserve &content, int tipo)
{
     msgbuf mensajeRecibido; // estructura vacía para recibir el mensaje

     int result = msgrcv( id, (void*) &mensajeRecibido, sizeof ( mensajeRecibido ), tipo ,0); // se espera recibir el mensaje de prioridad 1

     if ( -1 == result )
     {
         perror("No se pudo recibir algún mensaje");         
         _exit(1);
     }
     else
     {
         content = mensajeRecibido.messageContent;
         return 0;
     }
}
