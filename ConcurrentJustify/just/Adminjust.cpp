#include "Adminjust.h"

AdminJust::AdminJust()
    :messageControl()
    ,semProcesses()
{

}


AdminJust::~AdminJust()
{

}

int AdminJust::run( std::string listOfFiles[], int numberOfFiles ,short identationSize )
{
    Justify justIndividual;
    Reserv local;

    for ( int index = 0; index < numberOfFiles; ++ index )
    {
        if ( !fork() )
        {
            justIndividual.run( index + 1, messageControl, listOfFiles [ index ], listOfFiles [ index ] , identationSize );
            _exit(0);
        }
    }


    // se leen los mensajes enviados por cada hijo
    std::cout<<"Soy el papa"<<std::endl;
    Reserv mensajeRecibido;

    for ( short amountOfWords = 0; amountOfWords < Stadistics::AMOUNT_OF_WORDS; ++amountOfWords )
    {
        for ( int childNumber = 0; childNumber < numberOfFiles; ++ childNumber )
        {
            messageControl.Recibir( mensajeRecibido, childNumber + 1 );
            std:: cerr << "Hijo: "<< childNumber + 1 <<std::endl;
            std:: cerr << "Palabra: "<<mensajeRecibido.p <<std::endl;
            std:: cerr << "Cantidad: "<<mensajeRecibido.count <<std::endl<<std::endl;
        }
    }
    return 0;
}
