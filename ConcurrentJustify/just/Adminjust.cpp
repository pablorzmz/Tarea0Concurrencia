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

    std::cout<<"soy papa de todos "<<"\n";

    return 0;
}
