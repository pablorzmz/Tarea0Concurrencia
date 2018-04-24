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
    int i = 12;

    for ( int index = 0; index < numberOfFiles; ++ index )
    {
        if ( !fork() )
        {
            justIndividual.run( index + 1, messageControl, listOfFiles [ index ], listOfFiles [ index ] , identationSize );
            _exit(0);
        }else
        {
            wait(&i);
        }
    }

    std::cout<<"soy papa de todos "<< i <<" \n";
    Reserv tem;
    messageControl.Recibir( tem, 1);
    std::cout<< tem.p<< " tipo: "<<tem.c <<"\n";
    messageControl.Recibir( tem, 1);
    std::cout<< tem.p<< " tipo: "<<tem.c <<"\n";

    return 0;
}
