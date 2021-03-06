#include "Adminjust.h"

AdminJust::AdminJust()
    :idSharedMem(1)
    ,messageControl()
    ,semProcesses()
    ,individualJust()
{

}

AdminJust::~AdminJust()
{

}

// Método que realiza la creación de la memoria compartida para mensajes
int AdminJust::initSharedMem()
{
    this->idSharedMem = shmget( KEY3, sizeof( sharedMemStruct ),0600 | IPC_CREAT );
    if ( -1 ==  this->idSharedMem )
    {
        return this->idSharedMem; // si hay error, retorna -1
    }
    else
    {
        // enlazamos el puntero a esa memoria creada
        this->attachedSharedMem = (sharedMemStruct*) shmat( idSharedMem , NULL, 0 );
    }
    return 0;
}

// Método que realiza el hijo para mostrar los resultados leidos desde la memoria compartida
void AdminJust::printStadisticsFromFile( const char* alphchars, const short* wordsPerChar, const size_t amountOfLetters )
{
    // se utiliza un indice secundario para mostrar en orden de letra las palbras que se encuentren en memoria
    // con forme se vayan teniendo
    int subIndex = 0;
    for ( size_t index = 0; index < amountOfLetters; ++ index )
    {
        // se espera a aque el proceso padre de el visto bueno para mostrar los valores
        this->semProcesses.Wait();
        std::cout<<"Palabras que empiezan con la letra: "<<alphchars [ index ]<< std::endl;

        for ( int c = subIndex; c < (subIndex + wordsPerChar [ index ]); ++c )
        {
            std::cout<<"Palabra: "<<this->attachedSharedMem->r[ subIndex ].p<<std::endl;
            std::cout<<"\tAparece "<<this->attachedSharedMem->r [ subIndex ].count<<" veces."<<std::endl;
        }
        std::cout<<std::endl<<std::endl;
        subIndex += wordsPerChar [ index ];
    }
    _exit(0);
}

// método que llena el vector en cada iteración con los mensajes que envían los procesos hijos
void AdminJust::fillMessagesVector ( Reserved childMessages[][Stadistics::AMOUNT_OF_WORDS], size_t &childIndex, size_t &recordAllWords,  int& numberOfFiles )
{
    Reserved receivedMessage;
    // primero se llena el vector en donde van a acumularse los mensajes
    for ( int childNumber = 0; childNumber < numberOfFiles; ++ childNumber )
    {
        messageControl.Receive( receivedMessage, childNumber+ 1 );
        childMessages[ childNumber +1 ] [ childIndex ] = receivedMessage;

        // verificamos si ya algún hijo terminó
        for ( int childFinish = 0; childFinish < numberOfFiles; ++ childFinish )
        {
            messageControl.Receive( receivedMessage, ENDTYPEOP , IPC_NOWAIT );

            if ( receivedMessage.endOperations == true )
            {
                std::cout << receivedMessage.p <<std::endl;
                receivedMessage.endOperations = false;
            }
        }
    }

    // se actualizan los indices de las sublistas. Ahora va por el siguiente
    ++childIndex;

    // se actualizan las palabras recorridas
    ++recordAllWords;
}

// método que se ejecuta una vez que estén todas las palabras de una letra
// escribe en la memoria compartida la cantidad de apariciones de cada palabra
// organizandolas por letra de manera alfabética.
void AdminJust::updateSharedMemory(int & lastPointerToWords, Reserved childMessages[][Stadistics::AMOUNT_OF_WORDS], const short wordsPerChar[], size_t & currentLetterIndex, const  int& numberOfFiles)
{
    Reserved receivedMessage;
    // por cada palabra asociada a la letra contamos cuentas veces está por hijo
    for (int index = lastPointerToWords; index < (lastPointerToWords + wordsPerChar [ currentLetterIndex ]); ++index )
    {
        // se limpia la cadena de caracteres de la estructura del mensaje y la
        // varibale de conteo de la misma.
        for (int clean = 0; clean < TAM; ++ clean)
            receivedMessage.p[ clean ] = '\0';

        receivedMessage.count = 0;

        // copio la palabra para enviarla

        strncpy( receivedMessage.p, childMessages [ 1 ] [ index ].p, strlen( childMessages [ 1 ] [ index ].p ) );
        // por cada hijo busco y sumo esa palabra

        for (int subindex = 0; subindex < numberOfFiles; ++ subindex )
        {
            // sumo las veces que esta aparecce
            receivedMessage.count += childMessages [ subindex + 1 ] [ index ].count;
        }

        this->attachedSharedMem->r[this->attachedSharedMem->nCurrentTotals] = receivedMessage;
        ++this->attachedSharedMem->nCurrentTotals;
    }
    // se indica que el hijo tiene derecho a imprimir
    this->semProcesses.Signal();
    // cambiamos la posición en el vector
    lastPointerToWords += wordsPerChar[ currentLetterIndex ];
    ++currentLetterIndex;
}

int AdminJust::run( std::string listOfFiles[], int numberOfFiles ,short identationSize )
{
    for ( int index = 0; index < numberOfFiles; ++ index )
    {
        if ( !fork() )
        {
            this->individualJust.run( index + 1, messageControl, listOfFiles [ index ], listOfFiles [ index ] , identationSize );
            _exit(0);
        }
    }

    // se leen los mensajes enviados por cada hijo
    Reserved childMessages[ numberOfFiles+1 ][ Stadistics::AMOUNT_OF_WORDS ]; // vector que almacena los mensajes de cada tipo de hijo
    size_t amountOfChars = 20; // cantidad de letras del alfabeto
    size_t currentCharIndex = 0; //indica la letra actual del vector de letras que se está verificando
    size_t childIndex = 0; // control de los indices para organizar la sublista de mensajes por hijo
    size_t recordAllWords = 0; // verifica que solamente se recorran todas las palabras una vez
    int lastPointerToWords = 0; // guadar el orden por donde queda en cada subvector de mensajes de los hijos

    const char alphChars[ amountOfChars ] =
    {
        'a','b','c','d','e','f','g','i','l','m','n','o','p','r','s','t','u','v','w','x'
    };

    const short wordsPerChar[ amountOfChars ] =
    {
        /*a*/6, /*b*/4, /*c*/11, /*d*/6, /*e*/5, /*f*/4, /*g*/1, /*i*/3, /*l*/1, /*m*/1, /*n*/6, /*o**/3, /*p*/3, /*r*/3, /*s*/10, /*t*/ 9, /*u*/3, /*v*/3, /*w*/2, /*x*/2
    };

    // se inicia la memoria compartida
    int isValidSH= this->initSharedMem();

    if ( -1 == isValidSH )
    {
        perror( "ADMINJUST::No se pudo enlazar la memoria compartida");
    }
    else
    {
        // necesitamos tener el id del hijo que imprime para esperarlo por si tarda más en
        // imprimir las palabras
        int printerChildId = fork();

        if (! printerChildId  )
        {
            printStadisticsFromFile(  alphChars,  wordsPerChar, amountOfChars );
        }
        else
        {
            for ( int superIndex = 0; superIndex < Stadistics::AMOUNT_OF_WORDS; ++superIndex )
            {
                // se controla que el recivir mensajes solo se de una vez por cada hijo
                // y n veces por cada palabra
                if (recordAllWords < Stadistics::AMOUNT_OF_WORDS )
                {
                    fillMessagesVector( childMessages,  childIndex, recordAllWords, numberOfFiles );
                }

                // se verifica si ya están todas la palabras de la letra actual para enviarlas a
                // imprimir mediante el hijo que esta esperando
                int signalCounterValue = wordsPerChar [ currentCharIndex ] + lastPointerToWords  - 1 ;
                if ( superIndex == signalCounterValue )
                {
                    updateSharedMemory( lastPointerToWords, childMessages,  wordsPerChar,  currentCharIndex , numberOfFiles);
                }
            }
        }

        // esperamos a que el hijo que imprime termine su ejecución
        int x;
        waitpid( printerChildId, &x , 0 );

        // finalmente solo el papá libera la memoria compartida.
        shmdt( this->attachedSharedMem ) ;
        shmctl( this->idSharedMem, IPC_RMID, NULL );
    }
    return 0;
}
#include "Adminjust.h"

AdminJust::AdminJust()
    :idSharedMem(1)
    ,messageControl()
    ,semProcesses()
    ,individualJust()
{

}

AdminJust::~AdminJust()
{

}

// Método que realiza la creación de la memoria compartida para mensajes
int AdminJust::initSharedMem()
{
    this->idSharedMem = shmget( KEY3, sizeof( sharedMemStruct ),0600 | IPC_CREAT );
    if ( -1 ==  this->idSharedMem )
    {
        return this->idSharedMem; // si hay error, retorna -1
    }
    else
    {
        // enlazamos el puntero a esa memoria creada
        this->attachedSharedMem = (sharedMemStruct*) shmat( idSharedMem , NULL, 0 );
    }
    return 0;
}

// Método que realiza el hijo para mostrar los resultados leidos desde la memoria compartida
void AdminJust::printStadisticsFromFile( const char* alphchars, const short* wordsPerChar, const size_t amountOfLetters )
{
    // se utiliza un indice secundario para mostrar en orden de letra las palbras que se encuentren en memoria
    // con forme se vayan teniendo
    int subIndex = 0;
    for ( size_t index = 0; index < amountOfLetters; ++ index )
    {
        // se espera a aque el proceso padre de el visto bueno para mostrar los valores
        this->semProcesses.Wait();
        std::cout<<"Palabras que empiezan con la letra: "<<alphchars [ index ]<< std::endl;

        for ( int c = subIndex; c < (subIndex + wordsPerChar [ index ]); ++c )
        {
            std::cout<<"Palabra: "<<this->attachedSharedMem->r[ subIndex ].p<<std::endl;
            std::cout<<"\tAparece "<<this->attachedSharedMem->r [ subIndex ].count<<" veces."<<std::endl;
        }
        std::cout<<std::endl<<std::endl;
        subIndex += wordsPerChar [ index ];
    }
    _exit(0);
}

// método que llena el vector en cada iteración con los mensajes que envían los procesos hijos
void AdminJust::fillMessagesVector ( Reserved childMessages[][Stadistics::AMOUNT_OF_WORDS], size_t &childIndex, size_t &recordAllWords,  int& numberOfFiles )
{
    Reserved receivedMessage;
    // primero se llena el vector en donde van a acumularse los mensajes
    for ( int childNumber = 0; childNumber < numberOfFiles; ++ childNumber )
    {
        messageControl.Receive( receivedMessage, childNumber+ 1 );
        childMessages[ childNumber +1 ] [ childIndex ] = receivedMessage;

        // verificamos si ya algún hijo terminó
        for ( int childFinish = 0; childFinish < numberOfFiles; ++ childFinish )
        {
            messageControl.Receive( receivedMessage, ENDTYPEOP , IPC_NOWAIT );

            if ( receivedMessage.endOperations == true )
            {
                std::cout << receivedMessage.p <<std::endl;
                receivedMessage.endOperations = false;
            }
        }
    }

    // se actualizan los indices de las sublistas. Ahora va por el siguiente
    ++childIndex;

    // se actualizan las palabras recorridas
    ++recordAllWords;
}

// método que se ejecuta una vez que estén todas las palabras de una letra
// escribe en la memoria compartida la cantidad de apariciones de cada palabra
// organizandolas por letra de manera alfabética.
void AdminJust::updateSharedMemory(int & lastPointerToWords, Reserved childMessages[][Stadistics::AMOUNT_OF_WORDS], const short wordsPerChar[], size_t & currentLetterIndex, const  int& numberOfFiles)
{
    Reserved receivedMessage;
    // por cada palabra asociada a la letra contamos cuentas veces está por hijo
    for (int index = lastPointerToWords; index < (lastPointerToWords + wordsPerChar [ currentLetterIndex ]); ++index )
    {
        // se limpia la cadena de caracteres de la estructura del mensaje y la
        // varibale de conteo de la misma.
        for (int clean = 0; clean < TAM; ++ clean)
            receivedMessage.p[ clean ] = '\0';

        receivedMessage.count = 0;

        // copio la palabra para enviarla

        strncpy( receivedMessage.p, childMessages [ 1 ] [ index ].p, strlen( childMessages [ 1 ] [ index ].p ) );
        // por cada hijo busco y sumo esa palabra

        for (int subindex = 0; subindex < numberOfFiles; ++ subindex )
        {
            // sumo las veces que esta aparecce
            receivedMessage.count += childMessages [ subindex + 1 ] [ index ].count;
        }

        this->attachedSharedMem->r[this->attachedSharedMem->nCurrentTotals] = receivedMessage;
        ++this->attachedSharedMem->nCurrentTotals;
    }
    // se indica que el hijo tiene derecho a imprimir
    this->semProcesses.Signal();
    // cambiamos la posición en el vector
    lastPointerToWords += wordsPerChar[ currentLetterIndex ];
    ++currentLetterIndex;
}

int AdminJust::run( std::string listOfFiles[], int numberOfFiles ,short identationSize )
{
    for ( int index = 0; index < numberOfFiles; ++ index )
    {
        if ( !fork() )
        {
            this->individualJust.run( index + 1, messageControl, listOfFiles [ index ], listOfFiles [ index ] , identationSize );
            _exit(0);
        }
    }

    // se leen los mensajes enviados por cada hijo
    Reserved childMessages[ numberOfFiles+1 ][ Stadistics::AMOUNT_OF_WORDS ]; // vector que almacena los mensajes de cada tipo de hijo
    size_t amountOfChars = 20; // cantidad de letras del alfabeto
    size_t currentCharIndex = 0; //indica la letra actual del vector de letras que se está verificando
    size_t childIndex = 0; // control de los indices para organizar la sublista de mensajes por hijo
    size_t recordAllWords = 0; // verifica que solamente se recorran todas las palabras una vez
    int lastPointerToWords = 0; // guadar el orden por donde queda en cada subvector de mensajes de los hijos

    const char alphChars[ amountOfChars ] =
    {
        'a','b','c','d','e','f','g','i','l','m','n','o','p','r','s','t','u','v','w','x'
    };

    const short wordsPerChar[ amountOfChars ] =
    {
        /*a*/6, /*b*/4, /*c*/11, /*d*/6, /*e*/5, /*f*/4, /*g*/1, /*i*/3, /*l*/1, /*m*/1, /*n*/6, /*o**/3, /*p*/3, /*r*/3, /*s*/10, /*t*/ 9, /*u*/3, /*v*/3, /*w*/2, /*x*/2
    };

    // se inicia la memoria compartida
    int isValidSH= this->initSharedMem();

    if ( -1 == isValidSH )
    {
        perror( "ADMINJUST::No se pudo enlazar la memoria compartida");
    }
    else
    {
        // necesitamos tener el id del hijo que imprime para esperarlo por si tarda más en
        // imprimir las palabras
        int printerChildId = fork();

        if (! printerChildId  )
        {
            printStadisticsFromFile(  alphChars,  wordsPerChar, amountOfChars );
        }
        else
        {
            for ( int superIndex = 0; superIndex < Stadistics::AMOUNT_OF_WORDS; ++superIndex )
            {
                // se controla que el recivir mensajes solo se de una vez por cada hijo
                // y n veces por cada palabra
                if (recordAllWords < Stadistics::AMOUNT_OF_WORDS )
                {
                    fillMessagesVector( childMessages,  childIndex, recordAllWords, numberOfFiles );
                }

                // se verifica si ya están todas la palabras de la letra actual para enviarlas a
                // imprimir mediante el hijo que esta esperando
                int signalCounterValue = wordsPerChar [ currentCharIndex ] + lastPointerToWords  - 1 ;
                if ( superIndex == signalCounterValue )
                {
                    updateSharedMemory( lastPointerToWords, childMessages,  wordsPerChar,  currentCharIndex , numberOfFiles);
                }
            }
        }

        // esperamos a que el hijo que imprime termine su ejecución
        int x;
        waitpid( printerChildId, &x , 0 );

        // finalmente solo el papá libera la memoria compartida.
        shmdt( this->attachedSharedMem ) ;
        shmctl( this->idSharedMem, IPC_RMID, NULL );
    }
    return 0;
}

#include "Adminjust.h"

AdminJust::AdminJust()
    :idSharedMem(1)
    ,messageControl()
    ,semProcesses()
    ,individualJust()
{

}

AdminJust::~AdminJust()
{

}

// Método que realiza la creación de la memoria compartida para mensajes
int AdminJust::initSharedMem()
{
    this->idSharedMem = shmget( KEY3, sizeof( sharedMemStruct ),0600 | IPC_CREAT );
    if ( -1 ==  this->idSharedMem )
    {
        return this->idSharedMem; // si hay error, retorna -1
    }
    else
    {
        // enlazamos el puntero a esa memoria creada
        this->attachedSharedMem = (sharedMemStruct*) shmat( idSharedMem , NULL, 0 );
    }
    return 0;
}

// Método que realiza el hijo para mostrar los resultados leidos desde la memoria compartida
void AdminJust::printStadisticsFromFile( const char* alphchars, const short* wordsPerChar, const size_t amountOfLetters )
{
    // se utiliza un indice secundario para mostrar en orden de letra las palbras que se encuentren en memoria
    // con forme se vayan teniendo
    int subIndex = 0;
    for ( size_t index = 0; index < amountOfLetters; ++ index )
    {
        // se espera a aque el proceso padre de el visto bueno para mostrar los valores
        this->semProcesses.Wait();
        std::cout<<"Palabras que empiezan con la letra: "<<alphchars [ index ]<< std::endl;

        for ( int c = subIndex; c < (subIndex + wordsPerChar [ index ]); ++c )
        {
            std::cout<<"Palabra: "<<this->attachedSharedMem->r[ subIndex ].p<<std::endl;
            std::cout<<"\tAparece "<<this->attachedSharedMem->r [ subIndex ].count<<" veces."<<std::endl;
        }
        std::cout<<std::endl<<std::endl;
        subIndex += wordsPerChar [ index ];
    }
    _exit(0);
}

// método que llena el vector en cada iteración con los mensajes que envían los procesos hijos
void AdminJust::fillMessagesVector ( Reserved childMessages[][Stadistics::AMOUNT_OF_WORDS], size_t &childIndex, size_t &recordAllWords,  int& numberOfFiles )
{
    Reserved receivedMessage;
    // primero se llena el vector en donde van a acumularse los mensajes
    for ( int childNumber = 0; childNumber < numberOfFiles; ++ childNumber )
    {
        messageControl.Receive( receivedMessage, childNumber+ 1 );
        childMessages[ childNumber +1 ] [ childIndex ] = receivedMessage;

        // verificamos si ya algún hijo terminó
        for ( int childFinish = 0; childFinish < numberOfFiles; ++ childFinish )
        {
            messageControl.Receive( receivedMessage, ENDTYPEOP , IPC_NOWAIT );

            if ( receivedMessage.endOperations == true )
            {
                std::cout << receivedMessage.p <<std::endl;
                receivedMessage.endOperations = false;
            }
        }
    }

    // se actualizan los indices de las sublistas. Ahora va por el siguiente
    ++childIndex;

    // se actualizan las palabras recorridas
    ++recordAllWords;
}

// método que se ejecuta una vez que estén todas las palabras de una letra
// escribe en la memoria compartida la cantidad de apariciones de cada palabra
// organizandolas por letra de manera alfabética.
void AdminJust::updateSharedMemory(int & lastPointerToWords, Reserved childMessages[][Stadistics::AMOUNT_OF_WORDS], const short wordsPerChar[], size_t & currentLetterIndex, const  int& numberOfFiles)
{
    Reserved receivedMessage;
    // por cada palabra asociada a la letra contamos cuentas veces está por hijo
    for (int index = lastPointerToWords; index < (lastPointerToWords + wordsPerChar [ currentLetterIndex ]); ++index )
    {
        // se limpia la cadena de caracteres de la estructura del mensaje y la
        // varibale de conteo de la misma.
        for (int clean = 0; clean < TAM; ++ clean)
            receivedMessage.p[ clean ] = '\0';

        receivedMessage.count = 0;

        // copio la palabra para enviarla

        strncpy( receivedMessage.p, childMessages [ 1 ] [ index ].p, strlen( childMessages [ 1 ] [ index ].p ) );
        // por cada hijo busco y sumo esa palabra

        for (int subindex = 0; subindex < numberOfFiles; ++ subindex )
        {
            // sumo las veces que esta aparecce
            receivedMessage.count += childMessages [ subindex + 1 ] [ index ].count;
        }

        this->attachedSharedMem->r[this->attachedSharedMem->nCurrentTotals] = receivedMessage;
        ++this->attachedSharedMem->nCurrentTotals;
    }
    // se indica que el hijo tiene derecho a imprimir
    this->semProcesses.Signal();
    // cambiamos la posición en el vector
    lastPointerToWords += wordsPerChar[ currentLetterIndex ];
    ++currentLetterIndex;
}

int AdminJust::run( std::string listOfFiles[], int numberOfFiles ,short identationSize )
{
    for ( int index = 0; index < numberOfFiles; ++ index )
    {
        if ( !fork() )
        {
            this->individualJust.run( index + 1, messageControl, listOfFiles [ index ], listOfFiles [ index ] , identationSize );
            _exit(0);
        }
    }

    // se leen los mensajes enviados por cada hijo
    Reserved childMessages[ numberOfFiles+1 ][ Stadistics::AMOUNT_OF_WORDS ]; // vector que almacena los mensajes de cada tipo de hijo
    size_t amountOfChars = 20; // cantidad de letras del alfabeto
    size_t currentCharIndex = 0; //indica la letra actual del vector de letras que se está verificando
    size_t childIndex = 0; // control de los indices para organizar la sublista de mensajes por hijo
    size_t recordAllWords = 0; // verifica que solamente se recorran todas las palabras una vez
    int lastPointerToWords = 0; // guadar el orden por donde queda en cada subvector de mensajes de los hijos

    const char alphChars[ amountOfChars ] =
    {
        'a','b','c','d','e','f','g','i','l','m','n','o','p','r','s','t','u','v','w','x'
    };

    const short wordsPerChar[ amountOfChars ] =
    {
        /*a*/6, /*b*/4, /*c*/11, /*d*/6, /*e*/5, /*f*/4, /*g*/1, /*i*/3, /*l*/1, /*m*/1, /*n*/6, /*o**/3, /*p*/3, /*r*/3, /*s*/10, /*t*/ 9, /*u*/3, /*v*/3, /*w*/2, /*x*/2
    };

    // se inicia la memoria compartida
    int isValidSH= this->initSharedMem();

    if ( -1 == isValidSH )
    {
        perror( "ADMINJUST::No se pudo enlazar la memoria compartida");
    }
    else
    {
        // necesitamos tener el id del hijo que imprime para esperarlo por si tarda más en
        // imprimir las palabras
        int printerChildId = fork();

        if (! printerChildId  )
        {
            printStadisticsFromFile(  alphChars,  wordsPerChar, amountOfChars );
        }
        else
        {
            for ( int superIndex = 0; superIndex < Stadistics::AMOUNT_OF_WORDS; ++superIndex )
            {
                // se controla que el recivir mensajes solo se de una vez por cada hijo
                // y n veces por cada palabra
                if (recordAllWords < Stadistics::AMOUNT_OF_WORDS )
                {
                    fillMessagesVector( childMessages,  childIndex, recordAllWords, numberOfFiles );
                }

                // se verifica si ya están todas la palabras de la letra actual para enviarlas a
                // imprimir mediante el hijo que esta esperando
                int signalCounterValue = wordsPerChar [ currentCharIndex ] + lastPointerToWords  - 1 ;
                if ( superIndex == signalCounterValue )
                {
                    updateSharedMemory( lastPointerToWords, childMessages,  wordsPerChar,  currentCharIndex , numberOfFiles);
                }
            }
        }

        // esperamos a que el hijo que imprime termine su ejecución
        int x;
        waitpid( printerChildId, &x , 0 );

        // finalmente solo el papá libera la memoria compartida.
        shmdt( this->attachedSharedMem ) ;
        shmctl( this->idSharedMem, IPC_RMID, NULL );
    }
    return 0;
}

#include "Adminjust.h"

AdminJust::AdminJust()
    :idSharedMem(1)
    ,messageControl()
    ,semProcesses()
    ,individualJust()
{

}

AdminJust::~AdminJust()
{

}

// Método que realiza la creación de la memoria compartida para mensajes
int AdminJust::initSharedMem()
{
    this->idSharedMem = shmget( KEY3, sizeof( sharedMemStruct ),0600 | IPC_CREAT );
    if ( -1 ==  this->idSharedMem )
    {
        return this->idSharedMem; // si hay error, retorna -1
    }
    else
    {
        // enlazamos el puntero a esa memoria creada
        this->attachedSharedMem = (sharedMemStruct*) shmat( idSharedMem , NULL, 0 );
    }
    return 0;
}

// Método que realiza el hijo para mostrar los resultados leidos desde la memoria compartida
void AdminJust::printStadisticsFromFile( const char* alphchars, const short* wordsPerChar, const size_t amountOfLetters )
{
    // se utiliza un indice secundario para mostrar en orden de letra las palbras que se encuentren en memoria
    // con forme se vayan teniendo
    int subIndex = 0;
    for ( size_t index = 0; index < amountOfLetters; ++ index )
    {
        // se espera a aque el proceso padre de el visto bueno para mostrar los valores
        this->semProcesses.Wait();
        std::cout<<"Palabras que empiezan con la letra: "<<alphchars [ index ]<< std::endl;

        for ( int c = subIndex; c < (subIndex + wordsPerChar [ index ]); ++c )
        {
            std::cout<<"Palabra: "<<this->attachedSharedMem->r[ subIndex ].p<<std::endl;
            std::cout<<"\tAparece "<<this->attachedSharedMem->r [ subIndex ].count<<" veces."<<std::endl;
        }
        std::cout<<std::endl<<std::endl;
        subIndex += wordsPerChar [ index ];
    }
    _exit(0);
}

// método que llena el vector en cada iteración con los mensajes que envían los procesos hijos
void AdminJust::fillMessagesVector ( Reserved childMessages[][Stadistics::AMOUNT_OF_WORDS], size_t &childIndex, size_t &recordAllWords,  int& numberOfFiles )
{
    Reserved receivedMessage;
    // primero se llena el vector en donde van a acumularse los mensajes
    for ( int childNumber = 0; childNumber < numberOfFiles; ++ childNumber )
    {
        messageControl.Receive( receivedMessage, childNumber+ 1 );
        childMessages[ childNumber +1 ] [ childIndex ] = receivedMessage;

        // verificamos si ya algún hijo terminó
        for ( int childFinish = 0; childFinish < numberOfFiles; ++ childFinish )
        {
            messageControl.Receive( receivedMessage, ENDTYPEOP , IPC_NOWAIT );

            if ( receivedMessage.endOperations == true )
            {
                std::cout << receivedMessage.p <<std::endl;
                receivedMessage.endOperations = false;
            }
        }
    }

    // se actualizan los indices de las sublistas. Ahora va por el siguiente
    ++childIndex;

    // se actualizan las palabras recorridas
    ++recordAllWords;
}

// método que se ejecuta una vez que estén todas las palabras de una letra
// escribe en la memoria compartida la cantidad de apariciones de cada palabra
// organizandolas por letra de manera alfabética.
void AdminJust::updateSharedMemory(int & lastPointerToWords, Reserved childMessages[][Stadistics::AMOUNT_OF_WORDS], const short wordsPerChar[], size_t & currentLetterIndex, const  int& numberOfFiles)
{
    Reserved receivedMessage;
    // por cada palabra asociada a la letra contamos cuentas veces está por hijo
    for (int index = lastPointerToWords; index < (lastPointerToWords + wordsPerChar [ currentLetterIndex ]); ++index )
    {
        // se limpia la cadena de caracteres de la estructura del mensaje y la
        // varibale de conteo de la misma.
        for (int clean = 0; clean < TAM; ++ clean)
            receivedMessage.p[ clean ] = '\0';

        receivedMessage.count = 0;

        // copio la palabra para enviarla

        strncpy( receivedMessage.p, childMessages [ 1 ] [ index ].p, strlen( childMessages [ 1 ] [ index ].p ) );
        // por cada hijo busco y sumo esa palabra

        for (int subindex = 0; subindex < numberOfFiles; ++ subindex )
        {
            // sumo las veces que esta aparecce
            receivedMessage.count += childMessages [ subindex + 1 ] [ index ].count;
        }

        this->attachedSharedMem->r[this->attachedSharedMem->nCurrentTotals] = receivedMessage;
        ++this->attachedSharedMem->nCurrentTotals;
    }
    // se indica que el hijo tiene derecho a imprimir
    this->semProcesses.Signal();
    // cambiamos la posición en el vector
    lastPointerToWords += wordsPerChar[ currentLetterIndex ];
    ++currentLetterIndex;
}

int AdminJust::run( std::string listOfFiles[], int numberOfFiles ,short identationSize )
{
    for ( int index = 0; index < numberOfFiles; ++ index )
    {
        if ( !fork() )
        {
            this->individualJust.run( index + 1, messageControl, listOfFiles [ index ], listOfFiles [ index ] , identationSize );
            _exit(0);
        }
    }

    // se leen los mensajes enviados por cada hijo
    Reserved childMessages[ numberOfFiles+1 ][ Stadistics::AMOUNT_OF_WORDS ]; // vector que almacena los mensajes de cada tipo de hijo
    size_t amountOfChars = 20; // cantidad de letras del alfabeto
    size_t currentCharIndex = 0; //indica la letra actual del vector de letras que se está verificando
    size_t childIndex = 0; // control de los indices para organizar la sublista de mensajes por hijo
    size_t recordAllWords = 0; // verifica que solamente se recorran todas las palabras una vez
    int lastPointerToWords = 0; // guadar el orden por donde queda en cada subvector de mensajes de los hijos

    const char alphChars[ amountOfChars ] =
    {
        'a','b','c','d','e','f','g','i','l','m','n','o','p','r','s','t','u','v','w','x'
    };

    const short wordsPerChar[ amountOfChars ] =
    {
        /*a*/6, /*b*/4, /*c*/11, /*d*/6, /*e*/5, /*f*/4, /*g*/1, /*i*/3, /*l*/1, /*m*/1, /*n*/6, /*o**/3, /*p*/3, /*r*/3, /*s*/10, /*t*/ 9, /*u*/3, /*v*/3, /*w*/2, /*x*/2
    };

    // se inicia la memoria compartida
    int isValidSH= this->initSharedMem();

    if ( -1 == isValidSH )
    {
        perror( "ADMINJUST::No se pudo enlazar la memoria compartida");
    }
    else
    {
        // necesitamos tener el id del hijo que imprime para esperarlo por si tarda más en
        // imprimir las palabras
        int printerChildId = fork();

        if (! printerChildId  )
        {
            printStadisticsFromFile(  alphChars,  wordsPerChar, amountOfChars );
        }
        else
        {
            for ( int superIndex = 0; superIndex < Stadistics::AMOUNT_OF_WORDS; ++superIndex )
            {
                // se controla que el recivir mensajes solo se de una vez por cada hijo
                // y n veces por cada palabra
                if (recordAllWords < Stadistics::AMOUNT_OF_WORDS )
                {
                    fillMessagesVector( childMessages,  childIndex, recordAllWords, numberOfFiles );
                }

                // se verifica si ya están todas la palabras de la letra actual para enviarlas a
                // imprimir mediante el hijo que esta esperando
                int signalCounterValue = wordsPerChar [ currentCharIndex ] + lastPointerToWords  - 1 ;
                if ( superIndex == signalCounterValue )
                {
                    updateSharedMemory( lastPointerToWords, childMessages,  wordsPerChar,  currentCharIndex , numberOfFiles);
                }
            }
        }

        // esperamos a que el hijo que imprime termine su ejecución
        int x;
        waitpid( printerChildId, &x , 0 );

        // finalmente solo el papá libera la memoria compartida.
        shmdt( this->attachedSharedMem ) ;
        shmctl( this->idSharedMem, IPC_RMID, NULL );
    }
    return 0;
}

#include "Adminjust.h"

AdminJust::AdminJust()
    :idSharedMem(1)
    ,messageControl()
    ,semProcesses()
    ,individualJust()
{

}

AdminJust::~AdminJust()
{

}

// Método que realiza la creación de la memoria compartida para mensajes
int AdminJust::initSharedMem()
{
    this->idSharedMem = shmget( KEY3, sizeof( sharedMemStruct ),0600 | IPC_CREAT );
    if ( -1 ==  this->idSharedMem )
    {
        return this->idSharedMem; // si hay error, retorna -1
    }
    else
    {
        // enlazamos el puntero a esa memoria creada
        this->attachedSharedMem = (sharedMemStruct*) shmat( idSharedMem , NULL, 0 );
    }
    return 0;
}

// Método que realiza el hijo para mostrar los resultados leidos desde la memoria compartida
void AdminJust::printStadisticsFromFile( const char* alphchars, const short* wordsPerChar, const size_t amountOfLetters )
{
    // se utiliza un indice secundario para mostrar en orden de letra las palbras que se encuentren en memoria
    // con forme se vayan teniendo
    int subIndex = 0;
    for ( size_t index = 0; index < amountOfLetters; ++ index )
    {
        // se espera a aque el proceso padre de el visto bueno para mostrar los valores
        this->semProcesses.Wait();
        std::cout<<"Palabras que empiezan con la letra: "<<alphchars [ index ]<< std::endl;

        for ( int c = subIndex; c < (subIndex + wordsPerChar [ index ]); ++c )
        {
            std::cout<<"Palabra: "<<this->attachedSharedMem->r[ subIndex ].p<<std::endl;
            std::cout<<"\tAparece "<<this->attachedSharedMem->r [ subIndex ].count<<" veces."<<std::endl;
        }
        std::cout<<std::endl<<std::endl;
        subIndex += wordsPerChar [ index ];
    }
    _exit(0);
}

// método que llena el vector en cada iteración con los mensajes que envían los procesos hijos
void AdminJust::fillMessagesVector ( Reserved childMessages[][Stadistics::AMOUNT_OF_WORDS], size_t &childIndex, size_t &recordAllWords,  int& numberOfFiles )
{
    Reserved receivedMessage;
    // primero se llena el vector en donde van a acumularse los mensajes
    for ( int childNumber = 0; childNumber < numberOfFiles; ++ childNumber )
    {
        messageControl.Receive( receivedMessage, childNumber+ 1 );
        childMessages[ childNumber +1 ] [ childIndex ] = receivedMessage;

        // verificamos si ya algún hijo terminó
        for ( int childFinish = 0; childFinish < numberOfFiles; ++ childFinish )
        {
            messageControl.Receive( receivedMessage, ENDTYPEOP , IPC_NOWAIT );

            if ( receivedMessage.endOperations == true )
            {
                std::cout << receivedMessage.p <<std::endl;
                receivedMessage.endOperations = false;
            }
        }
    }

    // se actualizan los indices de las sublistas. Ahora va por el siguiente
    ++childIndex;

    // se actualizan las palabras recorridas
    ++recordAllWords;
}

// método que se ejecuta una vez que estén todas las palabras de una letra
// escribe en la memoria compartida la cantidad de apariciones de cada palabra
// organizandolas por letra de manera alfabética.
void AdminJust::updateSharedMemory(int & lastPointerToWords, Reserved childMessages[][Stadistics::AMOUNT_OF_WORDS], const short wordsPerChar[], size_t & currentLetterIndex, const  int& numberOfFiles)
{
    Reserved receivedMessage;
    // por cada palabra asociada a la letra contamos cuentas veces está por hijo
    for (int index = lastPointerToWords; index < (lastPointerToWords + wordsPerChar [ currentLetterIndex ]); ++index )
    {
        // se limpia la cadena de caracteres de la estructura del mensaje y la
        // varibale de conteo de la misma.
        for (int clean = 0; clean < TAM; ++ clean)
            receivedMessage.p[ clean ] = '\0';

        receivedMessage.count = 0;

        // copio la palabra para enviarla

        strncpy( receivedMessage.p, childMessages [ 1 ] [ index ].p, strlen( childMessages [ 1 ] [ index ].p ) );
        // por cada hijo busco y sumo esa palabra

        for (int subindex = 0; subindex < numberOfFiles; ++ subindex )
        {
            // sumo las veces que esta aparecce
            receivedMessage.count += childMessages [ subindex + 1 ] [ index ].count;
        }

        this->attachedSharedMem->r[this->attachedSharedMem->nCurrentTotals] = receivedMessage;
        ++this->attachedSharedMem->nCurrentTotals;
    }
    // se indica que el hijo tiene derecho a imprimir
    this->semProcesses.Signal();
    // cambiamos la posición en el vector
    lastPointerToWords += wordsPerChar[ currentLetterIndex ];
    ++currentLetterIndex;
}

int AdminJust::run( std::string listOfFiles[], int numberOfFiles ,short identationSize )
{
    for ( int index = 0; index < numberOfFiles; ++ index )
    {
        if ( !fork() )
        {
            this->individualJust.run( index + 1, messageControl, listOfFiles [ index ], listOfFiles [ index ] , identationSize );
            _exit(0);
        }
    }

    // se leen los mensajes enviados por cada hijo
    Reserved childMessages[ numberOfFiles+1 ][ Stadistics::AMOUNT_OF_WORDS ]; // vector que almacena los mensajes de cada tipo de hijo
    size_t amountOfChars = 20; // cantidad de letras del alfabeto
    size_t currentCharIndex = 0; //indica la letra actual del vector de letras que se está verificando
    size_t childIndex = 0; // control de los indices para organizar la sublista de mensajes por hijo
    size_t recordAllWords = 0; // verifica que solamente se recorran todas las palabras una vez
    int lastPointerToWords = 0; // guadar el orden por donde queda en cada subvector de mensajes de los hijos

    const char alphChars[ amountOfChars ] =
    {
        'a','b','c','d','e','f','g','i','l','m','n','o','p','r','s','t','u','v','w','x'
    };

    const short wordsPerChar[ amountOfChars ] =
    {
        /*a*/6, /*b*/4, /*c*/11, /*d*/6, /*e*/5, /*f*/4, /*g*/1, /*i*/3, /*l*/1, /*m*/1, /*n*/6, /*o**/3, /*p*/3, /*r*/3, /*s*/10, /*t*/ 9, /*u*/3, /*v*/3, /*w*/2, /*x*/2
    };

    // se inicia la memoria compartida
    int isValidSH= this->initSharedMem();

    if ( -1 == isValidSH )
    {
        perror( "ADMINJUST::No se pudo enlazar la memoria compartida");
    }
    else
    {
        // necesitamos tener el id del hijo que imprime para esperarlo por si tarda más en
        // imprimir las palabras
        int printerChildId = fork();

        if (! printerChildId  )
        {
            printStadisticsFromFile(  alphChars,  wordsPerChar, amountOfChars );
        }
        else
        {
            for ( int superIndex = 0; superIndex < Stadistics::AMOUNT_OF_WORDS; ++superIndex )
            {
                // se controla que el recivir mensajes solo se de una vez por cada hijo
                // y n veces por cada palabra
                if (recordAllWords < Stadistics::AMOUNT_OF_WORDS )
                {
                    fillMessagesVector( childMessages,  childIndex, recordAllWords, numberOfFiles );
                }

                // se verifica si ya están todas la palabras de la letra actual para enviarlas a
                // imprimir mediante el hijo que esta esperando
                int signalCounterValue = wordsPerChar [ currentCharIndex ] + lastPointerToWords  - 1 ;
                if ( superIndex == signalCounterValue )
                {
                    updateSharedMemory( lastPointerToWords, childMessages,  wordsPerChar,  currentCharIndex , numberOfFiles);
                }
            }
        }

        // esperamos a que el hijo que imprime termine su ejecución
        int x;
        waitpid( printerChildId, &x , 0 );

        // finalmente solo el papá libera la memoria compartida.
        shmdt( this->attachedSharedMem ) ;
        shmctl( this->idSharedMem, IPC_RMID, NULL );
    }
    return 0;
}

#include "Adminjust.h"

AdminJust::AdminJust()
    :idSharedMem(1)
    ,messageControl()
    ,semProcesses()
    ,individualJust()
{

}

AdminJust::~AdminJust()
{

}

// Método que realiza la creación de la memoria compartida para mensajes
int AdminJust::initSharedMem()
{
    this->idSharedMem = shmget( KEY3, sizeof( sharedMemStruct ),0600 | IPC_CREAT );
    if ( -1 ==  this->idSharedMem )
    {
        return this->idSharedMem; // si hay error, retorna -1
    }
    else
    {
        // enlazamos el puntero a esa memoria creada
        this->attachedSharedMem = (sharedMemStruct*) shmat( idSharedMem , NULL, 0 );
    }
    return 0;
}

// Método que realiza el hijo para mostrar los resultados leidos desde la memoria compartida
void AdminJust::printStadisticsFromFile( const char* alphchars, const short* wordsPerChar, const size_t amountOfLetters )
{
    // se utiliza un indice secundario para mostrar en orden de letra las palbras que se encuentren en memoria
    // con forme se vayan teniendo
    int subIndex = 0;
    for ( size_t index = 0; index < amountOfLetters; ++ index )
    {
        // se espera a aque el proceso padre de el visto bueno para mostrar los valores
        this->semProcesses.Wait();
        std::cout<<"Palabras que empiezan con la letra: "<<alphchars [ index ]<< std::endl;

        for ( int c = subIndex; c < (subIndex + wordsPerChar [ index ]); ++c )
        {
            std::cout<<"Palabra: "<<this->attachedSharedMem->r[ subIndex ].p<<std::endl;
            std::cout<<"\tAparece "<<this->attachedSharedMem->r [ subIndex ].count<<" veces."<<std::endl;
        }
        std::cout<<std::endl<<std::endl;
        subIndex += wordsPerChar [ index ];
    }
    _exit(0);
}

// método que llena el vector en cada iteración con los mensajes que envían los procesos hijos
void AdminJust::fillMessagesVector ( Reserved childMessages[][Stadistics::AMOUNT_OF_WORDS], size_t &childIndex, size_t &recordAllWords,  int& numberOfFiles )
{
    Reserved receivedMessage;
    // primero se llena el vector en donde van a acumularse los mensajes
    for ( int childNumber = 0; childNumber < numberOfFiles; ++ childNumber )
    {
        messageControl.Receive( receivedMessage, childNumber+ 1 );
        childMessages[ childNumber +1 ] [ childIndex ] = receivedMessage;

        // verificamos si ya algún hijo terminó
        for ( int childFinish = 0; childFinish < numberOfFiles; ++ childFinish )
        {
            messageControl.Receive( receivedMessage, ENDTYPEOP , IPC_NOWAIT );

            if ( receivedMessage.endOperations == true )
            {
                std::cout << receivedMessage.p <<std::endl;
                receivedMessage.endOperations = false;
            }
        }
    }

    // se actualizan los indices de las sublistas. Ahora va por el siguiente
    ++childIndex;

    // se actualizan las palabras recorridas
    ++recordAllWords;
}

// método que se ejecuta una vez que estén todas las palabras de una letra
// escribe en la memoria compartida la cantidad de apariciones de cada palabra
// organizandolas por letra de manera alfabética.
void AdminJust::updateSharedMemory(int & lastPointerToWords, Reserved childMessages[][Stadistics::AMOUNT_OF_WORDS], const short wordsPerChar[], size_t & currentLetterIndex, const  int& numberOfFiles)
{
    Reserved receivedMessage;
    // por cada palabra asociada a la letra contamos cuentas veces está por hijo
    for (int index = lastPointerToWords; index < (lastPointerToWords + wordsPerChar [ currentLetterIndex ]); ++index )
    {
        // se limpia la cadena de caracteres de la estructura del mensaje y la
        // varibale de conteo de la misma.
        for (int clean = 0; clean < TAM; ++ clean)
            receivedMessage.p[ clean ] = '\0';

        receivedMessage.count = 0;

        // copio la palabra para enviarla

        strncpy( receivedMessage.p, childMessages [ 1 ] [ index ].p, strlen( childMessages [ 1 ] [ index ].p ) );
        // por cada hijo busco y sumo esa palabra

        for (int subindex = 0; subindex < numberOfFiles; ++ subindex )
        {
            // sumo las veces que esta aparecce
            receivedMessage.count += childMessages [ subindex + 1 ] [ index ].count;
        }

        this->attachedSharedMem->r[this->attachedSharedMem->nCurrentTotals] = receivedMessage;
        ++this->attachedSharedMem->nCurrentTotals;
    }
    // se indica que el hijo tiene derecho a imprimir
    this->semProcesses.Signal();
    // cambiamos la posición en el vector
    lastPointerToWords += wordsPerChar[ currentLetterIndex ];
    ++currentLetterIndex;
}

int AdminJust::run( std::string listOfFiles[], int numberOfFiles ,short identationSize )
{
    for ( int index = 0; index < numberOfFiles; ++ index )
    {
        if ( !fork() )
        {
            this->individualJust.run( index + 1, messageControl, listOfFiles [ index ], listOfFiles [ index ] , identationSize );
            _exit(0);
        }
    }

    // se leen los mensajes enviados por cada hijo
    Reserved childMessages[ numberOfFiles+1 ][ Stadistics::AMOUNT_OF_WORDS ]; // vector que almacena los mensajes de cada tipo de hijo
    size_t amountOfChars = 20; // cantidad de letras del alfabeto
    size_t currentCharIndex = 0; //indica la letra actual del vector de letras que se está verificando
    size_t childIndex = 0; // control de los indices para organizar la sublista de mensajes por hijo
    size_t recordAllWords = 0; // verifica que solamente se recorran todas las palabras una vez
    int lastPointerToWords = 0; // guadar el orden por donde queda en cada subvector de mensajes de los hijos

    const char alphChars[ amountOfChars ] =
    {
        'a','b','c','d','e','f','g','i','l','m','n','o','p','r','s','t','u','v','w','x'
    };

    const short wordsPerChar[ amountOfChars ] =
    {
        /*a*/6, /*b*/4, /*c*/11, /*d*/6, /*e*/5, /*f*/4, /*g*/1, /*i*/3, /*l*/1, /*m*/1, /*n*/6, /*o**/3, /*p*/3, /*r*/3, /*s*/10, /*t*/ 9, /*u*/3, /*v*/3, /*w*/2, /*x*/2
    };

    // se inicia la memoria compartida
    int isValidSH= this->initSharedMem();

    if ( -1 == isValidSH )
    {
        perror( "ADMINJUST::No se pudo enlazar la memoria compartida");
    }
    else
    {
        // necesitamos tener el id del hijo que imprime para esperarlo por si tarda más en
        // imprimir las palabras
        int printerChildId = fork();

        if (! printerChildId  )
        {
            printStadisticsFromFile(  alphChars,  wordsPerChar, amountOfChars );
        }
        else
        {
            for ( int superIndex = 0; superIndex < Stadistics::AMOUNT_OF_WORDS; ++superIndex )
            {
                // se controla que el recivir mensajes solo se de una vez por cada hijo
                // y n veces por cada palabra
                if (recordAllWords < Stadistics::AMOUNT_OF_WORDS )
                {
                    fillMessagesVector( childMessages,  childIndex, recordAllWords, numberOfFiles );
                }

                // se verifica si ya están todas la palabras de la letra actual para enviarlas a
                // imprimir mediante el hijo que esta esperando
                int signalCounterValue = wordsPerChar [ currentCharIndex ] + lastPointerToWords  - 1 ;
                if ( superIndex == signalCounterValue )
                {
                    updateSharedMemory( lastPointerToWords, childMessages,  wordsPerChar,  currentCharIndex , numberOfFiles);
                }
            }
        }

        // esperamos a que el hijo que imprime termine su ejecución
        int x;
        waitpid( printerChildId, &x , 0 );

        // finalmente solo el papá libera la memoria compartida.
        shmdt( this->attachedSharedMem ) ;
        shmctl( this->idSharedMem, IPC_RMID, NULL );
    }
    return 0;
}

#include "Adminjust.h"

AdminJust::AdminJust()
    :idSharedMem(1)
    ,messageControl()
    ,semProcesses()
    ,individualJust()
{

}

AdminJust::~AdminJust()
{

}

// Método que realiza la creación de la memoria compartida para mensajes
int AdminJust::initSharedMem()
{
    this->idSharedMem = shmget( KEY3, sizeof( sharedMemStruct ),0600 | IPC_CREAT );
    if ( -1 ==  this->idSharedMem )
    {
        return this->idSharedMem; // si hay error, retorna -1
    }
    else
    {
        // enlazamos el puntero a esa memoria creada
        this->attachedSharedMem = (sharedMemStruct*) shmat( idSharedMem , NULL, 0 );
    }
    return 0;
}

// Método que realiza el hijo para mostrar los resultados leidos desde la memoria compartida
void AdminJust::printStadisticsFromFile( const char* alphchars, const short* wordsPerChar, const size_t amountOfLetters )
{
    // se utiliza un indice secundario para mostrar en orden de letra las palbras que se encuentren en memoria
    // con forme se vayan teniendo
    int subIndex = 0;
    for ( size_t index = 0; index < amountOfLetters; ++ index )
    {
        // se espera a aque el proceso padre de el visto bueno para mostrar los valores
        this->semProcesses.Wait();
        std::cout<<"Palabras que empiezan con la letra: "<<alphchars [ index ]<< std::endl;

        for ( int c = subIndex; c < (subIndex + wordsPerChar [ index ]); ++c )
        {
            std::cout<<"Palabra: "<<this->attachedSharedMem->r[ subIndex ].p<<std::endl;
            std::cout<<"\tAparece "<<this->attachedSharedMem->r [ subIndex ].count<<" veces."<<std::endl;
        }
        std::cout<<std::endl<<std::endl;
        subIndex += wordsPerChar [ index ];
    }
    _exit(0);
}

// método que llena el vector en cada iteración con los mensajes que envían los procesos hijos
void AdminJust::fillMessagesVector ( Reserved childMessages[][Stadistics::AMOUNT_OF_WORDS], size_t &childIndex, size_t &recordAllWords,  int& numberOfFiles )
{
    Reserved receivedMessage;
    // primero se llena el vector en donde van a acumularse los mensajes
    for ( int childNumber = 0; childNumber < numberOfFiles; ++ childNumber )
    {
        messageControl.Receive( receivedMessage, childNumber+ 1 );
        childMessages[ childNumber +1 ] [ childIndex ] = receivedMessage;

        // verificamos si ya algún hijo terminó
        for ( int childFinish = 0; childFinish < numberOfFiles; ++ childFinish )
        {
            messageControl.Receive( receivedMessage, ENDTYPEOP , IPC_NOWAIT );

            if ( receivedMessage.endOperations == true )
            {
                std::cout << receivedMessage.p <<std::endl;
                receivedMessage.endOperations = false;
            }
        }
    }

    // se actualizan los indices de las sublistas. Ahora va por el siguiente
    ++childIndex;

    // se actualizan las palabras recorridas
    ++recordAllWords;
}

// método que se ejecuta una vez que estén todas las palabras de una letra
// escribe en la memoria compartida la cantidad de apariciones de cada palabra
// organizandolas por letra de manera alfabética.
void AdminJust::updateSharedMemory(int & lastPointerToWords, Reserved childMessages[][Stadistics::AMOUNT_OF_WORDS], const short wordsPerChar[], size_t & currentLetterIndex, const  int& numberOfFiles)
{
    Reserved receivedMessage;
    // por cada palabra asociada a la letra contamos cuentas veces está por hijo
    for (int index = lastPointerToWords; index < (lastPointerToWords + wordsPerChar [ currentLetterIndex ]); ++index )
    {
        // se limpia la cadena de caracteres de la estructura del mensaje y la
        // varibale de conteo de la misma.
        for (int clean = 0; clean < TAM; ++ clean)
            receivedMessage.p[ clean ] = '\0';

        receivedMessage.count = 0;

        // copio la palabra para enviarla

        strncpy( receivedMessage.p, childMessages [ 1 ] [ index ].p, strlen( childMessages [ 1 ] [ index ].p ) );
        // por cada hijo busco y sumo esa palabra

        for (int subindex = 0; subindex < numberOfFiles; ++ subindex )
        {
            // sumo las veces que esta aparecce
            receivedMessage.count += childMessages [ subindex + 1 ] [ index ].count;
        }

        this->attachedSharedMem->r[this->attachedSharedMem->nCurrentTotals] = receivedMessage;
        ++this->attachedSharedMem->nCurrentTotals;
    }
    // se indica que el hijo tiene derecho a imprimir
    this->semProcesses.Signal();
    // cambiamos la posición en el vector
    lastPointerToWords += wordsPerChar[ currentLetterIndex ];
    ++currentLetterIndex;
}

int AdminJust::run( std::string listOfFiles[], int numberOfFiles ,short identationSize )
{
    for ( int index = 0; index < numberOfFiles; ++ index )
    {
        if ( !fork() )
        {
            this->individualJust.run( index + 1, messageControl, listOfFiles [ index ], listOfFiles [ index ] , identationSize );
            _exit(0);
        }
    }

    // se leen los mensajes enviados por cada hijo
    Reserved childMessages[ numberOfFiles+1 ][ Stadistics::AMOUNT_OF_WORDS ]; // vector que almacena los mensajes de cada tipo de hijo
    size_t amountOfChars = 20; // cantidad de letras del alfabeto
    size_t currentCharIndex = 0; //indica la letra actual del vector de letras que se está verificando
    size_t childIndex = 0; // control de los indices para organizar la sublista de mensajes por hijo
    size_t recordAllWords = 0; // verifica que solamente se recorran todas las palabras una vez
    int lastPointerToWords = 0; // guadar el orden por donde queda en cada subvector de mensajes de los hijos

    const char alphChars[ amountOfChars ] =
    {
        'a','b','c','d','e','f','g','i','l','m','n','o','p','r','s','t','u','v','w','x'
    };

    const short wordsPerChar[ amountOfChars ] =
    {
        /*a*/6, /*b*/4, /*c*/11, /*d*/6, /*e*/5, /*f*/4, /*g*/1, /*i*/3, /*l*/1, /*m*/1, /*n*/6, /*o**/3, /*p*/3, /*r*/3, /*s*/10, /*t*/ 9, /*u*/3, /*v*/3, /*w*/2, /*x*/2
    };

    // se inicia la memoria compartida
    int isValidSH= this->initSharedMem();

    if ( -1 == isValidSH )
    {
        perror( "ADMINJUST::No se pudo enlazar la memoria compartida");
    }
    else
    {
        // necesitamos tener el id del hijo que imprime para esperarlo por si tarda más en
        // imprimir las palabras
        int printerChildId = fork();

        if (! printerChildId  )
        {
            printStadisticsFromFile(  alphChars,  wordsPerChar, amountOfChars );
        }
        else
        {
            for ( int superIndex = 0; superIndex < Stadistics::AMOUNT_OF_WORDS; ++superIndex )
            {
                // se controla que el recivir mensajes solo se de una vez por cada hijo
                // y n veces por cada palabra
                if (recordAllWords < Stadistics::AMOUNT_OF_WORDS )
                {
                    fillMessagesVector( childMessages,  childIndex, recordAllWords, numberOfFiles );
                }

                // se verifica si ya están todas la palabras de la letra actual para enviarlas a
                // imprimir mediante el hijo que esta esperando
                int signalCounterValue = wordsPerChar [ currentCharIndex ] + lastPointerToWords  - 1 ;
                if ( superIndex == signalCounterValue )
                {
                    updateSharedMemory( lastPointerToWords, childMessages,  wordsPerChar,  currentCharIndex , numberOfFiles);
                }
            }
        }

        // esperamos a que el hijo que imprime termine su ejecución
        int x;
        waitpid( printerChildId, &x , 0 );

        // finalmente solo el papá libera la memoria compartida.
        shmdt( this->attachedSharedMem ) ;
        shmctl( this->idSharedMem, IPC_RMID, NULL );
    }
    return 0;
}

#include "Adminjust.h"

AdminJust::AdminJust()
    :idSharedMem(1)
    ,messageControl()
    ,semProcesses()
    ,individualJust()
{

}

AdminJust::~AdminJust()
{

}

// Método que realiza la creación de la memoria compartida para mensajes
int AdminJust::initSharedMem()
{
    this->idSharedMem = shmget( KEY3, sizeof( sharedMemStruct ),0600 | IPC_CREAT );
    if ( -1 ==  this->idSharedMem )
    {
        return this->idSharedMem; // si hay error, retorna -1
    }
    else
    {
        // enlazamos el puntero a esa memoria creada
        this->attachedSharedMem = (sharedMemStruct*) shmat( idSharedMem , NULL, 0 );
    }
    return 0;
}

// Método que realiza el hijo para mostrar los resultados leidos desde la memoria compartida
void AdminJust::printStadisticsFromFile( const char* alphchars, const short* wordsPerChar, const size_t amountOfLetters )
{
    // se utiliza un indice secundario para mostrar en orden de letra las palbras que se encuentren en memoria
    // con forme se vayan teniendo
    int subIndex = 0;
    for ( size_t index = 0; index < amountOfLetters; ++ index )
    {
        // se espera a aque el proceso padre de el visto bueno para mostrar los valores
        this->semProcesses.Wait();
        std::cout<<"Palabras que empiezan con la letra: "<<alphchars [ index ]<< std::endl;

        for ( int c = subIndex; c < (subIndex + wordsPerChar [ index ]); ++c )
        {
            std::cout<<"Palabra: "<<this->attachedSharedMem->r[ subIndex ].p<<std::endl;
            std::cout<<"\tAparece "<<this->attachedSharedMem->r [ subIndex ].count<<" veces."<<std::endl;
        }
        std::cout<<std::endl<<std::endl;
        subIndex += wordsPerChar [ index ];
    }
    _exit(0);
}

// método que llena el vector en cada iteración con los mensajes que envían los procesos hijos
void AdminJust::fillMessagesVector ( Reserved childMessages[][Stadistics::AMOUNT_OF_WORDS], size_t &childIndex, size_t &recordAllWords,  int& numberOfFiles )
{
    Reserved receivedMessage;
    // primero se llena el vector en donde van a acumularse los mensajes
    for ( int childNumber = 0; childNumber < numberOfFiles; ++ childNumber )
    {
        messageControl.Receive( receivedMessage, childNumber+ 1 );
        childMessages[ childNumber +1 ] [ childIndex ] = receivedMessage;

        // verificamos si ya algún hijo terminó
        for ( int childFinish = 0; childFinish < numberOfFiles; ++ childFinish )
        {
            messageControl.Receive( receivedMessage, ENDTYPEOP , IPC_NOWAIT );

            if ( receivedMessage.endOperations == true )
            {
                std::cout << receivedMessage.p <<std::endl;
                receivedMessage.endOperations = false;
            }
        }
    }

    // se actualizan los indices de las sublistas. Ahora va por el siguiente
    ++childIndex;

    // se actualizan las palabras recorridas
    ++recordAllWords;
}

// método que se ejecuta una vez que estén todas las palabras de una letra
// escribe en la memoria compartida la cantidad de apariciones de cada palabra
// organizandolas por letra de manera alfabética.
void AdminJust::updateSharedMemory(int & lastPointerToWords, Reserved childMessages[][Stadistics::AMOUNT_OF_WORDS], const short wordsPerChar[], size_t & currentLetterIndex, const  int& numberOfFiles)
{
    Reserved receivedMessage;
    // por cada palabra asociada a la letra contamos cuentas veces está por hijo
    for (int index = lastPointerToWords; index < (lastPointerToWords + wordsPerChar [ currentLetterIndex ]); ++index )
    {
        // se limpia la cadena de caracteres de la estructura del mensaje y la
        // varibale de conteo de la misma.
        for (int clean = 0; clean < TAM; ++ clean)
            receivedMessage.p[ clean ] = '\0';

        receivedMessage.count = 0;

        // copio la palabra para enviarla

        strncpy( receivedMessage.p, childMessages [ 1 ] [ index ].p, strlen( childMessages [ 1 ] [ index ].p ) );
        // por cada hijo busco y sumo esa palabra

        for (int subindex = 0; subindex < numberOfFiles; ++ subindex )
        {
            // sumo las veces que esta aparecce
            receivedMessage.count += childMessages [ subindex + 1 ] [ index ].count;
        }

        this->attachedSharedMem->r[this->attachedSharedMem->nCurrentTotals] = receivedMessage;
        ++this->attachedSharedMem->nCurrentTotals;
    }
    // se indica que el hijo tiene derecho a imprimir
    this->semProcesses.Signal();
    // cambiamos la posición en el vector
    lastPointerToWords += wordsPerChar[ currentLetterIndex ];
    ++currentLetterIndex;
}

int AdminJust::run( std::string listOfFiles[], int numberOfFiles ,short identationSize )
{
    for ( int index = 0; index < numberOfFiles; ++ index )
    {
        if ( !fork() )
        {
            this->individualJust.run( index + 1, messageControl, listOfFiles [ index ], listOfFiles [ index ] , identationSize );
            _exit(0);
        }
    }

    // se leen los mensajes enviados por cada hijo
    Reserved childMessages[ numberOfFiles+1 ][ Stadistics::AMOUNT_OF_WORDS ]; // vector que almacena los mensajes de cada tipo de hijo
    size_t amountOfChars = 20; // cantidad de letras del alfabeto
    size_t currentCharIndex = 0; //indica la letra actual del vector de letras que se está verificando
    size_t childIndex = 0; // control de los indices para organizar la sublista de mensajes por hijo
    size_t recordAllWords = 0; // verifica que solamente se recorran todas las palabras una vez
    int lastPointerToWords = 0; // guadar el orden por donde queda en cada subvector de mensajes de los hijos

    const char alphChars[ amountOfChars ] =
    {
        'a','b','c','d','e','f','g','i','l','m','n','o','p','r','s','t','u','v','w','x'
    };

    const short wordsPerChar[ amountOfChars ] =
    {
        /*a*/6, /*b*/4, /*c*/11, /*d*/6, /*e*/5, /*f*/4, /*g*/1, /*i*/3, /*l*/1, /*m*/1, /*n*/6, /*o**/3, /*p*/3, /*r*/3, /*s*/10, /*t*/ 9, /*u*/3, /*v*/3, /*w*/2, /*x*/2
    };

    // se inicia la memoria compartida
    int isValidSH= this->initSharedMem();

    if ( -1 == isValidSH )
    {
        perror( "ADMINJUST::No se pudo enlazar la memoria compartida");
    }
    else
    {
        // necesitamos tener el id del hijo que imprime para esperarlo por si tarda más en
        // imprimir las palabras
        int printerChildId = fork();

        if (! printerChildId  )
        {
            printStadisticsFromFile(  alphChars,  wordsPerChar, amountOfChars );
        }
        else
        {
            for ( int superIndex = 0; superIndex < Stadistics::AMOUNT_OF_WORDS; ++superIndex )
            {
                // se controla que el recivir mensajes solo se de una vez por cada hijo
                // y n veces por cada palabra
                if (recordAllWords < Stadistics::AMOUNT_OF_WORDS )
                {
                    fillMessagesVector( childMessages,  childIndex, recordAllWords, numberOfFiles );
                }

                // se verifica si ya están todas la palabras de la letra actual para enviarlas a
                // imprimir mediante el hijo que esta esperando
                int signalCounterValue = wordsPerChar [ currentCharIndex ] + lastPointerToWords  - 1 ;
                if ( superIndex == signalCounterValue )
                {
                    updateSharedMemory( lastPointerToWords, childMessages,  wordsPerChar,  currentCharIndex , numberOfFiles);
                }
            }
        }

        // esperamos a que el hijo que imprime termine su ejecución
        int x;
        waitpid( printerChildId, &x , 0 );

        // finalmente solo el papá libera la memoria compartida.
        shmdt( this->attachedSharedMem ) ;
        shmctl( this->idSharedMem, IPC_RMID, NULL );
    }
    return 0;
}

#include "Adminjust.h"

AdminJust::AdminJust()
    :idSharedMem(1)
    ,messageControl()
    ,semProcesses()
    ,individualJust()
{

}

AdminJust::~AdminJust()
{

}

// Método que realiza la creación de la memoria compartida para mensajes
int AdminJust::initSharedMem()
{
    this->idSharedMem = shmget( KEY3, sizeof( sharedMemStruct ),0600 | IPC_CREAT );
    if ( -1 ==  this->idSharedMem )
    {
        return this->idSharedMem; // si hay error, retorna -1
    }
    else
    {
        // enlazamos el puntero a esa memoria creada
        this->attachedSharedMem = (sharedMemStruct*) shmat( idSharedMem , NULL, 0 );
    }
    return 0;
}

// Método que realiza el hijo para mostrar los resultados leidos desde la memoria compartida
void AdminJust::printStadisticsFromFile( const char* alphchars, const short* wordsPerChar, const size_t amountOfLetters )
{
    // se utiliza un indice secundario para mostrar en orden de letra las palbras que se encuentren en memoria
    // con forme se vayan teniendo
    int subIndex = 0;
    for ( size_t index = 0; index < amountOfLetters; ++ index )
    {
        // se espera a aque el proceso padre de el visto bueno para mostrar los valores
        this->semProcesses.Wait();
        std::cout<<"Palabras que empiezan con la letra: "<<alphchars [ index ]<< std::endl;

        for ( int c = subIndex; c < (subIndex + wordsPerChar [ index ]); ++c )
        {
            std::cout<<"Palabra: "<<this->attachedSharedMem->r[ subIndex ].p<<std::endl;
            std::cout<<"\tAparece "<<this->attachedSharedMem->r [ subIndex ].count<<" veces."<<std::endl;
        }
        std::cout<<std::endl<<std::endl;
        subIndex += wordsPerChar [ index ];
    }
    _exit(0);
}

// método que llena el vector en cada iteración con los mensajes que envían los procesos hijos
void AdminJust::fillMessagesVector ( Reserved childMessages[][Stadistics::AMOUNT_OF_WORDS], size_t &childIndex, size_t &recordAllWords,  int& numberOfFiles )
{
    Reserved receivedMessage;
    // primero se llena el vector en donde van a acumularse los mensajes
    for ( int childNumber = 0; childNumber < numberOfFiles; ++ childNumber )
    {
        messageControl.Receive( receivedMessage, childNumber+ 1 );
        childMessages[ childNumber +1 ] [ childIndex ] = receivedMessage;

        // verificamos si ya algún hijo terminó
        for ( int childFinish = 0; childFinish < numberOfFiles; ++ childFinish )
        {
            messageControl.Receive( receivedMessage, ENDTYPEOP , IPC_NOWAIT );

            if ( receivedMessage.endOperations == true )
            {
                std::cout << receivedMessage.p <<std::endl;
                receivedMessage.endOperations = false;
            }
        }
    }

    // se actualizan los indices de las sublistas. Ahora va por el siguiente
    ++childIndex;

    // se actualizan las palabras recorridas
    ++recordAllWords;
}

// método que se ejecuta una vez que estén todas las palabras de una letra
// escribe en la memoria compartida la cantidad de apariciones de cada palabra
// organizandolas por letra de manera alfabética.
void AdminJust::updateSharedMemory(int & lastPointerToWords, Reserved childMessages[][Stadistics::AMOUNT_OF_WORDS], const short wordsPerChar[], size_t & currentLetterIndex, const  int& numberOfFiles)
{
    Reserved receivedMessage;
    // por cada palabra asociada a la letra contamos cuentas veces está por hijo
    for (int index = lastPointerToWords; index < (lastPointerToWords + wordsPerChar [ currentLetterIndex ]); ++index )
    {
        // se limpia la cadena de caracteres de la estructura del mensaje y la
        // varibale de conteo de la misma.
        for (int clean = 0; clean < TAM; ++ clean)
            receivedMessage.p[ clean ] = '\0';

        receivedMessage.count = 0;

        // copio la palabra para enviarla

        strncpy( receivedMessage.p, childMessages [ 1 ] [ index ].p, strlen( childMessages [ 1 ] [ index ].p ) );
        // por cada hijo busco y sumo esa palabra

        for (int subindex = 0; subindex < numberOfFiles; ++ subindex )
        {
            // sumo las veces que esta aparecce
            receivedMessage.count += childMessages [ subindex + 1 ] [ index ].count;
        }

        this->attachedSharedMem->r[this->attachedSharedMem->nCurrentTotals] = receivedMessage;
        ++this->attachedSharedMem->nCurrentTotals;
    }
    // se indica que el hijo tiene derecho a imprimir
    this->semProcesses.Signal();
    // cambiamos la posición en el vector
    lastPointerToWords += wordsPerChar[ currentLetterIndex ];
    ++currentLetterIndex;
}

int AdminJust::run( std::string listOfFiles[], int numberOfFiles ,short identationSize )
{
    for ( int index = 0; index < numberOfFiles; ++ index )
    {
        if ( !fork() )
        {
            this->individualJust.run( index + 1, messageControl, listOfFiles [ index ], listOfFiles [ index ] , identationSize );
            _exit(0);
        }
    }

    // se leen los mensajes enviados por cada hijo
    Reserved childMessages[ numberOfFiles+1 ][ Stadistics::AMOUNT_OF_WORDS ]; // vector que almacena los mensajes de cada tipo de hijo
    size_t amountOfChars = 20; // cantidad de letras del alfabeto
    size_t currentCharIndex = 0; //indica la letra actual del vector de letras que se está verificando
    size_t childIndex = 0; // control de los indices para organizar la sublista de mensajes por hijo
    size_t recordAllWords = 0; // verifica que solamente se recorran todas las palabras una vez
    int lastPointerToWords = 0; // guadar el orden por donde queda en cada subvector de mensajes de los hijos

    const char alphChars[ amountOfChars ] =
    {
        'a','b','c','d','e','f','g','i','l','m','n','o','p','r','s','t','u','v','w','x'
    };

    const short wordsPerChar[ amountOfChars ] =
    {
        /*a*/6, /*b*/4, /*c*/11, /*d*/6, /*e*/5, /*f*/4, /*g*/1, /*i*/3, /*l*/1, /*m*/1, /*n*/6, /*o**/3, /*p*/3, /*r*/3, /*s*/10, /*t*/ 9, /*u*/3, /*v*/3, /*w*/2, /*x*/2
    };

    // se inicia la memoria compartida
    int isValidSH= this->initSharedMem();

    if ( -1 == isValidSH )
    {
        perror( "ADMINJUST::No se pudo enlazar la memoria compartida");
    }
    else
    {
        // necesitamos tener el id del hijo que imprime para esperarlo por si tarda más en
        // imprimir las palabras
        int printerChildId = fork();

        if (! printerChildId  )
        {
            printStadisticsFromFile(  alphChars,  wordsPerChar, amountOfChars );
        }
        else
        {
            for ( int superIndex = 0; superIndex < Stadistics::AMOUNT_OF_WORDS; ++superIndex )
            {
                // se controla que el recivir mensajes solo se de una vez por cada hijo
                // y n veces por cada palabra
                if (recordAllWords < Stadistics::AMOUNT_OF_WORDS )
                {
                    fillMessagesVector( childMessages,  childIndex, recordAllWords, numberOfFiles );
                }

                // se verifica si ya están todas la palabras de la letra actual para enviarlas a
                // imprimir mediante el hijo que esta esperando
                int signalCounterValue = wordsPerChar [ currentCharIndex ] + lastPointerToWords  - 1 ;
                if ( superIndex == signalCounterValue )
                {
                    updateSharedMemory( lastPointerToWords, childMessages,  wordsPerChar,  currentCharIndex , numberOfFiles);
                }
            }
        }

        // esperamos a que el hijo que imprime termine su ejecución
        int x;
        waitpid( printerChildId, &x , 0 );

        // finalmente solo el papá libera la memoria compartida.
        shmdt( this->attachedSharedMem ) ;
        shmctl( this->idSharedMem, IPC_RMID, NULL );
    }
    return 0;
}

#include "Adminjust.h"

AdminJust::AdminJust()
    :idSharedMem(1)
    ,messageControl()
    ,semProcesses()
    ,individualJust()
{

}

AdminJust::~AdminJust()
{

}

// Método que realiza la creación de la memoria compartida para mensajes
int AdminJust::initSharedMem()
{
    this->idSharedMem = shmget( KEY3, sizeof( sharedMemStruct ),0600 | IPC_CREAT );
    if ( -1 ==  this->idSharedMem )
    {
        return this->idSharedMem; // si hay error, retorna -1
    }
    else
    {
        // enlazamos el puntero a esa memoria creada
        this->attachedSharedMem = (sharedMemStruct*) shmat( idSharedMem , NULL, 0 );
    }
    return 0;
}

// Método que realiza el hijo para mostrar los resultados leidos desde la memoria compartida
void AdminJust::printStadisticsFromFile( const char* alphchars, const short* wordsPerChar, const size_t amountOfLetters )
{
    // se utiliza un indice secundario para mostrar en orden de letra las palbras que se encuentren en memoria
    // con forme se vayan teniendo
    int subIndex = 0;
    for ( size_t index = 0; index < amountOfLetters; ++ index )
    {
        // se espera a aque el proceso padre de el visto bueno para mostrar los valores
        this->semProcesses.Wait();
        std::cout<<"Palabras que empiezan con la letra: "<<alphchars [ index ]<< std::endl;

        for ( int c = subIndex; c < (subIndex + wordsPerChar [ index ]); ++c )
        {
            std::cout<<"Palabra: "<<this->attachedSharedMem->r[ subIndex ].p<<std::endl;
            std::cout<<"\tAparece "<<this->attachedSharedMem->r [ subIndex ].count<<" veces."<<std::endl;
        }
        std::cout<<std::endl<<std::endl;
        subIndex += wordsPerChar [ index ];
    }
    _exit(0);
}

// método que llena el vector en cada iteración con los mensajes que envían los procesos hijos
void AdminJust::fillMessagesVector ( Reserved childMessages[][Stadistics::AMOUNT_OF_WORDS], size_t &childIndex, size_t &recordAllWords,  int& numberOfFiles )
{
    Reserved receivedMessage;
    // primero se llena el vector en donde van a acumularse los mensajes
    for ( int childNumber = 0; childNumber < numberOfFiles; ++ childNumber )
    {
        messageControl.Receive( receivedMessage, childNumber+ 1 );
        childMessages[ childNumber +1 ] [ childIndex ] = receivedMessage;

        // verificamos si ya algún hijo terminó
        for ( int childFinish = 0; childFinish < numberOfFiles; ++ childFinish )
        {
            messageControl.Receive( receivedMessage, ENDTYPEOP , IPC_NOWAIT );

            if ( receivedMessage.endOperations == true )
            {
                std::cout << receivedMessage.p <<std::endl;
                receivedMessage.endOperations = false;
            }
        }
    }

    // se actualizan los indices de las sublistas. Ahora va por el siguiente
    ++childIndex;

    // se actualizan las palabras recorridas
    ++recordAllWords;
}

// método que se ejecuta una vez que estén todas las palabras de una letra
// escribe en la memoria compartida la cantidad de apariciones de cada palabra
// organizandolas por letra de manera alfabética.
void AdminJust::updateSharedMemory(int & lastPointerToWords, Reserved childMessages[][Stadistics::AMOUNT_OF_WORDS], const short wordsPerChar[], size_t & currentLetterIndex, const  int& numberOfFiles)
{
    Reserved receivedMessage;
    // por cada palabra asociada a la letra contamos cuentas veces está por hijo
    for (int index = lastPointerToWords; index < (lastPointerToWords + wordsPerChar [ currentLetterIndex ]); ++index )
    {
        // se limpia la cadena de caracteres de la estructura del mensaje y la
        // varibale de conteo de la misma.
        for (int clean = 0; clean < TAM; ++ clean)
            receivedMessage.p[ clean ] = '\0';

        receivedMessage.count = 0;

        // copio la palabra para enviarla

        strncpy( receivedMessage.p, childMessages [ 1 ] [ index ].p, strlen( childMessages [ 1 ] [ index ].p ) );
        // por cada hijo busco y sumo esa palabra

        for (int subindex = 0; subindex < numberOfFiles; ++ subindex )
        {
            // sumo las veces que esta aparecce
            receivedMessage.count += childMessages [ subindex + 1 ] [ index ].count;
        }

        this->attachedSharedMem->r[this->attachedSharedMem->nCurrentTotals] = receivedMessage;
        ++this->attachedSharedMem->nCurrentTotals;
    }
    // se indica que el hijo tiene derecho a imprimir
    this->semProcesses.Signal();
    // cambiamos la posición en el vector
    lastPointerToWords += wordsPerChar[ currentLetterIndex ];
    ++currentLetterIndex;
}

int AdminJust::run( std::string listOfFiles[], int numberOfFiles ,short identationSize )
{
    for ( int index = 0; index < numberOfFiles; ++ index )
    {
        if ( !fork() )
        {
            this->individualJust.run( index + 1, messageControl, listOfFiles [ index ], listOfFiles [ index ] , identationSize );
            _exit(0);
        }
    }

    // se leen los mensajes enviados por cada hijo
    Reserved childMessages[ numberOfFiles+1 ][ Stadistics::AMOUNT_OF_WORDS ]; // vector que almacena los mensajes de cada tipo de hijo
    size_t amountOfChars = 20; // cantidad de letras del alfabeto
    size_t currentCharIndex = 0; //indica la letra actual del vector de letras que se está verificando
    size_t childIndex = 0; // control de los indices para organizar la sublista de mensajes por hijo
    size_t recordAllWords = 0; // verifica que solamente se recorran todas las palabras una vez
    int lastPointerToWords = 0; // guadar el orden por donde queda en cada subvector de mensajes de los hijos

    const char alphChars[ amountOfChars ] =
    {
        'a','b','c','d','e','f','g','i','l','m','n','o','p','r','s','t','u','v','w','x'
    };

    const short wordsPerChar[ amountOfChars ] =
    {
        /*a*/6, /*b*/4, /*c*/11, /*d*/6, /*e*/5, /*f*/4, /*g*/1, /*i*/3, /*l*/1, /*m*/1, /*n*/6, /*o**/3, /*p*/3, /*r*/3, /*s*/10, /*t*/ 9, /*u*/3, /*v*/3, /*w*/2, /*x*/2
    };

    // se inicia la memoria compartida
    int isValidSH= this->initSharedMem();

    if ( -1 == isValidSH )
    {
        perror( "ADMINJUST::No se pudo enlazar la memoria compartida");
    }
    else
    {
        // necesitamos tener el id del hijo que imprime para esperarlo por si tarda más en
        // imprimir las palabras
        int printerChildId = fork();

        if (! printerChildId  )
        {
            printStadisticsFromFile(  alphChars,  wordsPerChar, amountOfChars );
        }
        else
        {
            for ( int superIndex = 0; superIndex < Stadistics::AMOUNT_OF_WORDS; ++superIndex )
            {
                // se controla que el recivir mensajes solo se de una vez por cada hijo
                // y n veces por cada palabra
                if (recordAllWords < Stadistics::AMOUNT_OF_WORDS )
                {
                    fillMessagesVector( childMessages,  childIndex, recordAllWords, numberOfFiles );
                }

                // se verifica si ya están todas la palabras de la letra actual para enviarlas a
                // imprimir mediante el hijo que esta esperando
                int signalCounterValue = wordsPerChar [ currentCharIndex ] + lastPointerToWords  - 1 ;
                if ( superIndex == signalCounterValue )
                {
                    updateSharedMemory( lastPointerToWords, childMessages,  wordsPerChar,  currentCharIndex , numberOfFiles);
                }
            }
        }

        // esperamos a que el hijo que imprime termine su ejecución
        int x;
        waitpid( printerChildId, &x , 0 );

        // finalmente solo el papá libera la memoria compartida.
        shmdt( this->attachedSharedMem ) ;
        shmctl( this->idSharedMem, IPC_RMID, NULL );
    }
    return 0;
}

#include "Adminjust.h"

AdminJust::AdminJust()
    :idSharedMem(1)
    ,messageControl()
    ,semProcesses()
    ,individualJust()
{

}

AdminJust::~AdminJust()
{

}

// Método que realiza la creación de la memoria compartida para mensajes
int AdminJust::initSharedMem()
{
    this->idSharedMem = shmget( KEY3, sizeof( sharedMemStruct ),0600 | IPC_CREAT );
    if ( -1 ==  this->idSharedMem )
    {
        return this->idSharedMem; // si hay error, retorna -1
    }
    else
    {
        // enlazamos el puntero a esa memoria creada
        this->attachedSharedMem = (sharedMemStruct*) shmat( idSharedMem , NULL, 0 );
    }
    return 0;
}

// Método que realiza el hijo para mostrar los resultados leidos desde la memoria compartida
void AdminJust::printStadisticsFromFile( const char* alphchars, const short* wordsPerChar, const size_t amountOfLetters )
{
    // se utiliza un indice secundario para mostrar en orden de letra las palbras que se encuentren en memoria
    // con forme se vayan teniendo
    int subIndex = 0;
    for ( size_t index = 0; index < amountOfLetters; ++ index )
    {
        // se espera a aque el proceso padre de el visto bueno para mostrar los valores
        this->semProcesses.Wait();
        std::cout<<"Palabras que empiezan con la letra: "<<alphchars [ index ]<< std::endl;

        for ( int c = subIndex; c < (subIndex + wordsPerChar [ index ]); ++c )
        {
            std::cout<<"Palabra: "<<this->attachedSharedMem->r[ subIndex ].p<<std::endl;
            std::cout<<"\tAparece "<<this->attachedSharedMem->r [ subIndex ].count<<" veces."<<std::endl;
        }
        std::cout<<std::endl<<std::endl;
        subIndex += wordsPerChar [ index ];
    }
    _exit(0);
}

// método que llena el vector en cada iteración con los mensajes que envían los procesos hijos
void AdminJust::fillMessagesVector ( Reserved childMessages[][Stadistics::AMOUNT_OF_WORDS], size_t &childIndex, size_t &recordAllWords,  int& numberOfFiles )
{
    Reserved receivedMessage;
    // primero se llena el vector en donde van a acumularse los mensajes
    for ( int childNumber = 0; childNumber < numberOfFiles; ++ childNumber )
    {
        messageControl.Receive( receivedMessage, childNumber+ 1 );
        childMessages[ childNumber +1 ] [ childIndex ] = receivedMessage;

        // verificamos si ya algún hijo terminó
        for ( int childFinish = 0; childFinish < numberOfFiles; ++ childFinish )
        {
            messageControl.Receive( receivedMessage, ENDTYPEOP , IPC_NOWAIT );

            if ( receivedMessage.endOperations == true )
            {
                std::cout << receivedMessage.p <<std::endl;
                receivedMessage.endOperations = false;
            }
        }
    }

    // se actualizan los indices de las sublistas. Ahora va por el siguiente
    ++childIndex;

    // se actualizan las palabras recorridas
    ++recordAllWords;
}

// método que se ejecuta una vez que estén todas las palabras de una letra
// escribe en la memoria compartida la cantidad de apariciones de cada palabra
// organizandolas por letra de manera alfabética.
void AdminJust::updateSharedMemory(int & lastPointerToWords, Reserved childMessages[][Stadistics::AMOUNT_OF_WORDS], const short wordsPerChar[], size_t & currentLetterIndex, const  int& numberOfFiles)
{
    Reserved receivedMessage;
    // por cada palabra asociada a la letra contamos cuentas veces está por hijo
    for (int index = lastPointerToWords; index < (lastPointerToWords + wordsPerChar [ currentLetterIndex ]); ++index )
    {
        // se limpia la cadena de caracteres de la estructura del mensaje y la
        // varibale de conteo de la misma.
        for (int clean = 0; clean < TAM; ++ clean)
            receivedMessage.p[ clean ] = '\0';

        receivedMessage.count = 0;

        // copio la palabra para enviarla

        strncpy( receivedMessage.p, childMessages [ 1 ] [ index ].p, strlen( childMessages [ 1 ] [ index ].p ) );
        // por cada hijo busco y sumo esa palabra

        for (int subindex = 0; subindex < numberOfFiles; ++ subindex )
        {
            // sumo las veces que esta aparecce
            receivedMessage.count += childMessages [ subindex + 1 ] [ index ].count;
        }

        this->attachedSharedMem->r[this->attachedSharedMem->nCurrentTotals] = receivedMessage;
        ++this->attachedSharedMem->nCurrentTotals;
    }
    // se indica que el hijo tiene derecho a imprimir
    this->semProcesses.Signal();
    // cambiamos la posición en el vector
    lastPointerToWords += wordsPerChar[ currentLetterIndex ];
    ++currentLetterIndex;
}

int AdminJust::run( std::string listOfFiles[], int numberOfFiles ,short identationSize )
{
    for ( int index = 0; index < numberOfFiles; ++ index )
    {
        if ( !fork() )
        {
            this->individualJust.run( index + 1, messageControl, listOfFiles [ index ], listOfFiles [ index ] , identationSize );
            _exit(0);
        }
    }

    // se leen los mensajes enviados por cada hijo
    Reserved childMessages[ numberOfFiles+1 ][ Stadistics::AMOUNT_OF_WORDS ]; // vector que almacena los mensajes de cada tipo de hijo
    size_t amountOfChars = 20; // cantidad de letras del alfabeto
    size_t currentCharIndex = 0; //indica la letra actual del vector de letras que se está verificando
    size_t childIndex = 0; // control de los indices para organizar la sublista de mensajes por hijo
    size_t recordAllWords = 0; // verifica que solamente se recorran todas las palabras una vez
    int lastPointerToWords = 0; // guadar el orden por donde queda en cada subvector de mensajes de los hijos

    const char alphChars[ amountOfChars ] =
    {
        'a','b','c','d','e','f','g','i','l','m','n','o','p','r','s','t','u','v','w','x'
    };

    const short wordsPerChar[ amountOfChars ] =
    {
        /*a*/6, /*b*/4, /*c*/11, /*d*/6, /*e*/5, /*f*/4, /*g*/1, /*i*/3, /*l*/1, /*m*/1, /*n*/6, /*o**/3, /*p*/3, /*r*/3, /*s*/10, /*t*/ 9, /*u*/3, /*v*/3, /*w*/2, /*x*/2
    };

    // se inicia la memoria compartida
    int isValidSH= this->initSharedMem();

    if ( -1 == isValidSH )
    {
        perror( "ADMINJUST::No se pudo enlazar la memoria compartida");
    }
    else
    {
        // necesitamos tener el id del hijo que imprime para esperarlo por si tarda más en
        // imprimir las palabras
        int printerChildId = fork();

        if (! printerChildId  )
        {
            printStadisticsFromFile(  alphChars,  wordsPerChar, amountOfChars );
        }
        else
        {
            for ( int superIndex = 0; superIndex < Stadistics::AMOUNT_OF_WORDS; ++superIndex )
            {
                // se controla que el recivir mensajes solo se de una vez por cada hijo
                // y n veces por cada palabra
                if (recordAllWords < Stadistics::AMOUNT_OF_WORDS )
                {
                    fillMessagesVector( childMessages,  childIndex, recordAllWords, numberOfFiles );
                }

                // se verifica si ya están todas la palabras de la letra actual para enviarlas a
                // imprimir mediante el hijo que esta esperando
                int signalCounterValue = wordsPerChar [ currentCharIndex ] + lastPointerToWords  - 1 ;
                if ( superIndex == signalCounterValue )
                {
                    updateSharedMemory( lastPointerToWords, childMessages,  wordsPerChar,  currentCharIndex , numberOfFiles);
                }
            }
        }

        // esperamos a que el hijo que imprime termine su ejecución
        int x;
        waitpid( printerChildId, &x , 0 );

        // finalmente solo el papá libera la memoria compartida.
        shmdt( this->attachedSharedMem ) ;
        shmctl( this->idSharedMem, IPC_RMID, NULL );
    }
    return 0;
}

#include "Adminjust.h"

AdminJust::AdminJust()
    :idSharedMem(1)
    ,messageControl()
    ,semProcesses()
    ,individualJust()
{

}

AdminJust::~AdminJust()
{

}

// Método que realiza la creación de la memoria compartida para mensajes
int AdminJust::initSharedMem()
{
    this->idSharedMem = shmget( KEY3, sizeof( sharedMemStruct ),0600 | IPC_CREAT );
    if ( -1 ==  this->idSharedMem )
    {
        return this->idSharedMem; // si hay error, retorna -1
    }
    else
    {
        // enlazamos el puntero a esa memoria creada
        this->attachedSharedMem = (sharedMemStruct*) shmat( idSharedMem , NULL, 0 );
    }
    return 0;
}

// Método que realiza el hijo para mostrar los resultados leidos desde la memoria compartida
void AdminJust::printStadisticsFromFile( const char* alphchars, const short* wordsPerChar, const size_t amountOfLetters )
{
    // se utiliza un indice secundario para mostrar en orden de letra las palbras que se encuentren en memoria
    // con forme se vayan teniendo
    int subIndex = 0;
    for ( size_t index = 0; index < amountOfLetters; ++ index )
    {
        // se espera a aque el proceso padre de el visto bueno para mostrar los valores
        this->semProcesses.Wait();
        std::cout<<"Palabras que empiezan con la letra: "<<alphchars [ index ]<< std::endl;

        for ( int c = subIndex; c < (subIndex + wordsPerChar [ index ]); ++c )
        {
            std::cout<<"Palabra: "<<this->attachedSharedMem->r[ subIndex ].p<<std::endl;
            std::cout<<"\tAparece "<<this->attachedSharedMem->r [ subIndex ].count<<" veces."<<std::endl;
        }
        std::cout<<std::endl<<std::endl;
        subIndex += wordsPerChar [ index ];
    }
    _exit(0);
}

// método que llena el vector en cada iteración con los mensajes que envían los procesos hijos
void AdminJust::fillMessagesVector ( Reserved childMessages[][Stadistics::AMOUNT_OF_WORDS], size_t &childIndex, size_t &recordAllWords,  int& numberOfFiles )
{
    Reserved receivedMessage;
    // primero se llena el vector en donde van a acumularse los mensajes
    for ( int childNumber = 0; childNumber < numberOfFiles; ++ childNumber )
    {
        messageControl.Receive( receivedMessage, childNumber+ 1 );
        childMessages[ childNumber +1 ] [ childIndex ] = receivedMessage;

        // verificamos si ya algún hijo terminó
        for ( int childFinish = 0; childFinish < numberOfFiles; ++ childFinish )
        {
            messageControl.Receive( receivedMessage, ENDTYPEOP , IPC_NOWAIT );

            if ( receivedMessage.endOperations == true )
            {
                std::cout << receivedMessage.p <<std::endl;
                receivedMessage.endOperations = false;
            }
        }
    }

    // se actualizan los indices de las sublistas. Ahora va por el siguiente
    ++childIndex;

    // se actualizan las palabras recorridas
    ++recordAllWords;
}

// método que se ejecuta una vez que estén todas las palabras de una letra
// escribe en la memoria compartida la cantidad de apariciones de cada palabra
// organizandolas por letra de manera alfabética.
void AdminJust::updateSharedMemory(int & lastPointerToWords, Reserved childMessages[][Stadistics::AMOUNT_OF_WORDS], const short wordsPerChar[], size_t & currentLetterIndex, const  int& numberOfFiles)
{
    Reserved receivedMessage;
    // por cada palabra asociada a la letra contamos cuentas veces está por hijo
    for (int index = lastPointerToWords; index < (lastPointerToWords + wordsPerChar [ currentLetterIndex ]); ++index )
    {
        // se limpia la cadena de caracteres de la estructura del mensaje y la
        // varibale de conteo de la misma.
        for (int clean = 0; clean < TAM; ++ clean)
            receivedMessage.p[ clean ] = '\0';

        receivedMessage.count = 0;

        // copio la palabra para enviarla

        strncpy( receivedMessage.p, childMessages [ 1 ] [ index ].p, strlen( childMessages [ 1 ] [ index ].p ) );
        // por cada hijo busco y sumo esa palabra

        for (int subindex = 0; subindex < numberOfFiles; ++ subindex )
        {
            // sumo las veces que esta aparecce
            receivedMessage.count += childMessages [ subindex + 1 ] [ index ].count;
        }

        this->attachedSharedMem->r[this->attachedSharedMem->nCurrentTotals] = receivedMessage;
        ++this->attachedSharedMem->nCurrentTotals;
    }
    // se indica que el hijo tiene derecho a imprimir
    this->semProcesses.Signal();
    // cambiamos la posición en el vector
    lastPointerToWords += wordsPerChar[ currentLetterIndex ];
    ++currentLetterIndex;
}

int AdminJust::run( std::string listOfFiles[], int numberOfFiles ,short identationSize )
{
    for ( int index = 0; index < numberOfFiles; ++ index )
    {
        if ( !fork() )
        {
            this->individualJust.run( index + 1, messageControl, listOfFiles [ index ], listOfFiles [ index ] , identationSize );
            _exit(0);
        }
    }

    // se leen los mensajes enviados por cada hijo
    Reserved childMessages[ numberOfFiles+1 ][ Stadistics::AMOUNT_OF_WORDS ]; // vector que almacena los mensajes de cada tipo de hijo
    size_t amountOfChars = 20; // cantidad de letras del alfabeto
    size_t currentCharIndex = 0; //indica la letra actual del vector de letras que se está verificando
    size_t childIndex = 0; // control de los indices para organizar la sublista de mensajes por hijo
    size_t recordAllWords = 0; // verifica que solamente se recorran todas las palabras una vez
    int lastPointerToWords = 0; // guadar el orden por donde queda en cada subvector de mensajes de los hijos

    const char alphChars[ amountOfChars ] =
    {
        'a','b','c','d','e','f','g','i','l','m','n','o','p','r','s','t','u','v','w','x'
    };

    const short wordsPerChar[ amountOfChars ] =
    {
        /*a*/6, /*b*/4, /*c*/11, /*d*/6, /*e*/5, /*f*/4, /*g*/1, /*i*/3, /*l*/1, /*m*/1, /*n*/6, /*o**/3, /*p*/3, /*r*/3, /*s*/10, /*t*/ 9, /*u*/3, /*v*/3, /*w*/2, /*x*/2
    };

    // se inicia la memoria compartida
    int isValidSH= this->initSharedMem();

    if ( -1 == isValidSH )
    {
        perror( "ADMINJUST::No se pudo enlazar la memoria compartida");
    }
    else
    {
        // necesitamos tener el id del hijo que imprime para esperarlo por si tarda más en
        // imprimir las palabras
        int printerChildId = fork();

        if (! printerChildId  )
        {
            printStadisticsFromFile(  alphChars,  wordsPerChar, amountOfChars );
        }
        else
        {
            for ( int superIndex = 0; superIndex < Stadistics::AMOUNT_OF_WORDS; ++superIndex )
            {
                // se controla que el recivir mensajes solo se de una vez por cada hijo
                // y n veces por cada palabra
                if (recordAllWords < Stadistics::AMOUNT_OF_WORDS )
                {
                    fillMessagesVector( childMessages,  childIndex, recordAllWords, numberOfFiles );
                }

                // se verifica si ya están todas la palabras de la letra actual para enviarlas a
                // imprimir mediante el hijo que esta esperando
                int signalCounterValue = wordsPerChar [ currentCharIndex ] + lastPointerToWords  - 1 ;
                if ( superIndex == signalCounterValue )
                {
                    updateSharedMemory( lastPointerToWords, childMessages,  wordsPerChar,  currentCharIndex , numberOfFiles);
                }
            }
        }

        // esperamos a que el hijo que imprime termine su ejecución
        int x;
        waitpid( printerChildId, &x , 0 );

        // finalmente solo el papá libera la memoria compartida.
        shmdt( this->attachedSharedMem ) ;
        shmctl( this->idSharedMem, IPC_RMID, NULL );
    }
    return 0;
}

#include "Adminjust.h"

AdminJust::AdminJust()
    :idSharedMem(1)
    ,messageControl()
    ,semProcesses()
    ,individualJust()
{

}

AdminJust::~AdminJust()
{

}

// Método que realiza la creación de la memoria compartida para mensajes
int AdminJust::initSharedMem()
{
    this->idSharedMem = shmget( KEY3, sizeof( sharedMemStruct ),0600 | IPC_CREAT );
    if ( -1 ==  this->idSharedMem )
    {
        return this->idSharedMem; // si hay error, retorna -1
    }
    else
    {
        // enlazamos el puntero a esa memoria creada
        this->attachedSharedMem = (sharedMemStruct*) shmat( idSharedMem , NULL, 0 );
    }
    return 0;
}

// Método que realiza el hijo para mostrar los resultados leidos desde la memoria compartida
void AdminJust::printStadisticsFromFile( const char* alphchars, const short* wordsPerChar, const size_t amountOfLetters )
{
    // se utiliza un indice secundario para mostrar en orden de letra las palbras que se encuentren en memoria
    // con forme se vayan teniendo
    int subIndex = 0;
    for ( size_t index = 0; index < amountOfLetters; ++ index )
    {
        // se espera a aque el proceso padre de el visto bueno para mostrar los valores
        this->semProcesses.Wait();
        std::cout<<"Palabras que empiezan con la letra: "<<alphchars [ index ]<< std::endl;

        for ( int c = subIndex; c < (subIndex + wordsPerChar [ index ]); ++c )
        {
            std::cout<<"Palabra: "<<this->attachedSharedMem->r[ subIndex ].p<<std::endl;
            std::cout<<"\tAparece "<<this->attachedSharedMem->r [ subIndex ].count<<" veces."<<std::endl;
        }
        std::cout<<std::endl<<std::endl;
        subIndex += wordsPerChar [ index ];
    }
    _exit(0);
}

// método que llena el vector en cada iteración con los mensajes que envían los procesos hijos
void AdminJust::fillMessagesVector ( Reserved childMessages[][Stadistics::AMOUNT_OF_WORDS], size_t &childIndex, size_t &recordAllWords,  int& numberOfFiles )
{
    Reserved receivedMessage;
    // primero se llena el vector en donde van a acumularse los mensajes
    for ( int childNumber = 0; childNumber < numberOfFiles; ++ childNumber )
    {
        messageControl.Receive( receivedMessage, childNumber+ 1 );
        childMessages[ childNumber +1 ] [ childIndex ] = receivedMessage;

        // verificamos si ya algún hijo terminó
        for ( int childFinish = 0; childFinish < numberOfFiles; ++ childFinish )
        {
            messageControl.Receive( receivedMessage, ENDTYPEOP , IPC_NOWAIT );

            if ( receivedMessage.endOperations == true )
            {
                std::cout << receivedMessage.p <<std::endl;
                receivedMessage.endOperations = false;
            }
        }
    }

    // se actualizan los indices de las sublistas. Ahora va por el siguiente
    ++childIndex;

    // se actualizan las palabras recorridas
    ++recordAllWords;
}

// método que se ejecuta una vez que estén todas las palabras de una letra
// escribe en la memoria compartida la cantidad de apariciones de cada palabra
// organizandolas por letra de manera alfabética.
void AdminJust::updateSharedMemory(int & lastPointerToWords, Reserved childMessages[][Stadistics::AMOUNT_OF_WORDS], const short wordsPerChar[], size_t & currentLetterIndex, const  int& numberOfFiles)
{
    Reserved receivedMessage;
    // por cada palabra asociada a la letra contamos cuentas veces está por hijo
    for (int index = lastPointerToWords; index < (lastPointerToWords + wordsPerChar [ currentLetterIndex ]); ++index )
    {
        // se limpia la cadena de caracteres de la estructura del mensaje y la
        // varibale de conteo de la misma.
        for (int clean = 0; clean < TAM; ++ clean)
            receivedMessage.p[ clean ] = '\0';

        receivedMessage.count = 0;

        // copio la palabra para enviarla

        strncpy( receivedMessage.p, childMessages [ 1 ] [ index ].p, strlen( childMessages [ 1 ] [ index ].p ) );
        // por cada hijo busco y sumo esa palabra

        for (int subindex = 0; subindex < numberOfFiles; ++ subindex )
        {
            // sumo las veces que esta aparecce
            receivedMessage.count += childMessages [ subindex + 1 ] [ index ].count;
        }

        this->attachedSharedMem->r[this->attachedSharedMem->nCurrentTotals] = receivedMessage;
        ++this->attachedSharedMem->nCurrentTotals;
    }
    // se indica que el hijo tiene derecho a imprimir
    this->semProcesses.Signal();
    // cambiamos la posición en el vector
    lastPointerToWords += wordsPerChar[ currentLetterIndex ];
    ++currentLetterIndex;
}

int AdminJust::run( std::string listOfFiles[], int numberOfFiles ,short identationSize )
{
    for ( int index = 0; index < numberOfFiles; ++ index )
    {
        if ( !fork() )
        {
            this->individualJust.run( index + 1, messageControl, listOfFiles [ index ], listOfFiles [ index ] , identationSize );
            _exit(0);
        }
    }

    // se leen los mensajes enviados por cada hijo
    Reserved childMessages[ numberOfFiles+1 ][ Stadistics::AMOUNT_OF_WORDS ]; // vector que almacena los mensajes de cada tipo de hijo
    size_t amountOfChars = 20; // cantidad de letras del alfabeto
    size_t currentCharIndex = 0; //indica la letra actual del vector de letras que se está verificando
    size_t childIndex = 0; // control de los indices para organizar la sublista de mensajes por hijo
    size_t recordAllWords = 0; // verifica que solamente se recorran todas las palabras una vez
    int lastPointerToWords = 0; // guadar el orden por donde queda en cada subvector de mensajes de los hijos

    const char alphChars[ amountOfChars ] =
    {
        'a','b','c','d','e','f','g','i','l','m','n','o','p','r','s','t','u','v','w','x'
    };

    const short wordsPerChar[ amountOfChars ] =
    {
        /*a*/6, /*b*/4, /*c*/11, /*d*/6, /*e*/5, /*f*/4, /*g*/1, /*i*/3, /*l*/1, /*m*/1, /*n*/6, /*o**/3, /*p*/3, /*r*/3, /*s*/10, /*t*/ 9, /*u*/3, /*v*/3, /*w*/2, /*x*/2
    };

    // se inicia la memoria compartida
    int isValidSH= this->initSharedMem();

    if ( -1 == isValidSH )
    {
        perror( "ADMINJUST::No se pudo enlazar la memoria compartida");
    }
    else
    {
        // necesitamos tener el id del hijo que imprime para esperarlo por si tarda más en
        // imprimir las palabras
        int printerChildId = fork();

        if (! printerChildId  )
        {
            printStadisticsFromFile(  alphChars,  wordsPerChar, amountOfChars );
        }
        else
        {
            for ( int superIndex = 0; superIndex < Stadistics::AMOUNT_OF_WORDS; ++superIndex )
            {
                // se controla que el recivir mensajes solo se de una vez por cada hijo
                // y n veces por cada palabra
                if (recordAllWords < Stadistics::AMOUNT_OF_WORDS )
                {
                    fillMessagesVector( childMessages,  childIndex, recordAllWords, numberOfFiles );
                }

                // se verifica si ya están todas la palabras de la letra actual para enviarlas a
                // imprimir mediante el hijo que esta esperando
                int signalCounterValue = wordsPerChar [ currentCharIndex ] + lastPointerToWords  - 1 ;
                if ( superIndex == signalCounterValue )
                {
                    updateSharedMemory( lastPointerToWords, childMessages,  wordsPerChar,  currentCharIndex , numberOfFiles);
                }
            }
        }

        // esperamos a que el hijo que imprime termine su ejecución
        int x;
        waitpid( printerChildId, &x , 0 );

        // finalmente solo el papá libera la memoria compartida.
        shmdt( this->attachedSharedMem ) ;
        shmctl( this->idSharedMem, IPC_RMID, NULL );
    }
    return 0;
}

#include "Adminjust.h"

AdminJust::AdminJust()
    :idSharedMem(1)
    ,messageControl()
    ,semProcesses()
    ,individualJust()
{

}

AdminJust::~AdminJust()
{

}

// Método que realiza la creación de la memoria compartida para mensajes
int AdminJust::initSharedMem()
{
    this->idSharedMem = shmget( KEY3, sizeof( sharedMemStruct ),0600 | IPC_CREAT );
    if ( -1 ==  this->idSharedMem )
    {
        return this->idSharedMem; // si hay error, retorna -1
    }
    else
    {
        // enlazamos el puntero a esa memoria creada
        this->attachedSharedMem = (sharedMemStruct*) shmat( idSharedMem , NULL, 0 );
    }
    return 0;
}

// Método que realiza el hijo para mostrar los resultados leidos desde la memoria compartida
void AdminJust::printStadisticsFromFile( const char* alphchars, const short* wordsPerChar, const size_t amountOfLetters )
{
    // se utiliza un indice secundario para mostrar en orden de letra las palbras que se encuentren en memoria
    // con forme se vayan teniendo
    int subIndex = 0;
    for ( size_t index = 0; index < amountOfLetters; ++ index )
    {
        // se espera a aque el proceso padre de el visto bueno para mostrar los valores
        this->semProcesses.Wait();
        std::cout<<"Palabras que empiezan con la letra: "<<alphchars [ index ]<< std::endl;

        for ( int c = subIndex; c < (subIndex + wordsPerChar [ index ]); ++c )
        {
            std::cout<<"Palabra: "<<this->attachedSharedMem->r[ subIndex ].p<<std::endl;
            std::cout<<"\tAparece "<<this->attachedSharedMem->r [ subIndex ].count<<" veces."<<std::endl;
        }
        std::cout<<std::endl<<std::endl;
        subIndex += wordsPerChar [ index ];
    }
    _exit(0);
}

// método que llena el vector en cada iteración con los mensajes que envían los procesos hijos
void AdminJust::fillMessagesVector ( Reserved childMessages[][Stadistics::AMOUNT_OF_WORDS], size_t &childIndex, size_t &recordAllWords,  int& numberOfFiles )
{
    Reserved receivedMessage;
    // primero se llena el vector en donde van a acumularse los mensajes
    for ( int childNumber = 0; childNumber < numberOfFiles; ++ childNumber )
    {
        messageControl.Receive( receivedMessage, childNumber+ 1 );
        childMessages[ childNumber +1 ] [ childIndex ] = receivedMessage;

        // verificamos si ya algún hijo terminó
        for ( int childFinish = 0; childFinish < numberOfFiles; ++ childFinish )
        {
            messageControl.Receive( receivedMessage, ENDTYPEOP , IPC_NOWAIT );

            if ( receivedMessage.endOperations == true )
            {
                std::cout << receivedMessage.p <<std::endl;
                receivedMessage.endOperations = false;
            }
        }
    }

    // se actualizan los indices de las sublistas. Ahora va por el siguiente
    ++childIndex;

    // se actualizan las palabras recorridas
    ++recordAllWords;
}

// método que se ejecuta una vez que estén todas las palabras de una letra
// escribe en la memoria compartida la cantidad de apariciones de cada palabra
// organizandolas por letra de manera alfabética.
void AdminJust::updateSharedMemory(int & lastPointerToWords, Reserved childMessages[][Stadistics::AMOUNT_OF_WORDS], const short wordsPerChar[], size_t & currentLetterIndex, const  int& numberOfFiles)
{
    Reserved receivedMessage;
    // por cada palabra asociada a la letra contamos cuentas veces está por hijo
    for (int index = lastPointerToWords; index < (lastPointerToWords + wordsPerChar [ currentLetterIndex ]); ++index )
    {
        // se limpia la cadena de caracteres de la estructura del mensaje y la
        // varibale de conteo de la misma.
        for (int clean = 0; clean < TAM; ++ clean)
            receivedMessage.p[ clean ] = '\0';

        receivedMessage.count = 0;

        // copio la palabra para enviarla

        strncpy( receivedMessage.p, childMessages [ 1 ] [ index ].p, strlen( childMessages [ 1 ] [ index ].p ) );
        // por cada hijo busco y sumo esa palabra

        for (int subindex = 0; subindex < numberOfFiles; ++ subindex )
        {
            // sumo las veces que esta aparecce
            receivedMessage.count += childMessages [ subindex + 1 ] [ index ].count;
        }

        this->attachedSharedMem->r[this->attachedSharedMem->nCurrentTotals] = receivedMessage;
        ++this->attachedSharedMem->nCurrentTotals;
    }
    // se indica que el hijo tiene derecho a imprimir
    this->semProcesses.Signal();
    // cambiamos la posición en el vector
    lastPointerToWords += wordsPerChar[ currentLetterIndex ];
    ++currentLetterIndex;
}

int AdminJust::run( std::string listOfFiles[], int numberOfFiles ,short identationSize )
{
    for ( int index = 0; index < numberOfFiles; ++ index )
    {
        if ( !fork() )
        {
            this->individualJust.run( index + 1, messageControl, listOfFiles [ index ], listOfFiles [ index ] , identationSize );
            _exit(0);
        }
    }

    // se leen los mensajes enviados por cada hijo
    Reserved childMessages[ numberOfFiles+1 ][ Stadistics::AMOUNT_OF_WORDS ]; // vector que almacena los mensajes de cada tipo de hijo
    size_t amountOfChars = 20; // cantidad de letras del alfabeto
    size_t currentCharIndex = 0; //indica la letra actual del vector de letras que se está verificando
    size_t childIndex = 0; // control de los indices para organizar la sublista de mensajes por hijo
    size_t recordAllWords = 0; // verifica que solamente se recorran todas las palabras una vez
    int lastPointerToWords = 0; // guadar el orden por donde queda en cada subvector de mensajes de los hijos

    const char alphChars[ amountOfChars ] =
    {
        'a','b','c','d','e','f','g','i','l','m','n','o','p','r','s','t','u','v','w','x'
    };

    const short wordsPerChar[ amountOfChars ] =
    {
        /*a*/6, /*b*/4, /*c*/11, /*d*/6, /*e*/5, /*f*/4, /*g*/1, /*i*/3, /*l*/1, /*m*/1, /*n*/6, /*o**/3, /*p*/3, /*r*/3, /*s*/10, /*t*/ 9, /*u*/3, /*v*/3, /*w*/2, /*x*/2
    };

    // se inicia la memoria compartida
    int isValidSH= this->initSharedMem();

    if ( -1 == isValidSH )
    {
        perror( "ADMINJUST::No se pudo enlazar la memoria compartida");
    }
    else
    {
        // necesitamos tener el id del hijo que imprime para esperarlo por si tarda más en
        // imprimir las palabras
        int printerChildId = fork();

        if (! printerChildId  )
        {
            printStadisticsFromFile(  alphChars,  wordsPerChar, amountOfChars );
        }
        else
        {
            for ( int superIndex = 0; superIndex < Stadistics::AMOUNT_OF_WORDS; ++superIndex )
            {
                // se controla que el recivir mensajes solo se de una vez por cada hijo
                // y n veces por cada palabra
                if (recordAllWords < Stadistics::AMOUNT_OF_WORDS )
                {
                    fillMessagesVector( childMessages,  childIndex, recordAllWords, numberOfFiles );
                }

                // se verifica si ya están todas la palabras de la letra actual para enviarlas a
                // imprimir mediante el hijo que esta esperando
                int signalCounterValue = wordsPerChar [ currentCharIndex ] + lastPointerToWords  - 1 ;
                if ( superIndex == signalCounterValue )
                {
                    updateSharedMemory( lastPointerToWords, childMessages,  wordsPerChar,  currentCharIndex , numberOfFiles);
                }
            }
        }

        // esperamos a que el hijo que imprime termine su ejecución
        int x;
        waitpid( printerChildId, &x , 0 );

        // finalmente solo el papá libera la memoria compartida.
        shmdt( this->attachedSharedMem ) ;
        shmctl( this->idSharedMem, IPC_RMID, NULL );
    }
    return 0;
}

#include "Adminjust.h"

AdminJust::AdminJust()
    :idSharedMem(1)
    ,messageControl()
    ,semProcesses()
    ,individualJust()
{

}

AdminJust::~AdminJust()
{

}

// Método que realiza la creación de la memoria compartida para mensajes
int AdminJust::initSharedMem()
{
    this->idSharedMem = shmget( KEY3, sizeof( sharedMemStruct ),0600 | IPC_CREAT );
    if ( -1 ==  this->idSharedMem )
    {
        return this->idSharedMem; // si hay error, retorna -1
    }
    else
    {
        // enlazamos el puntero a esa memoria creada
        this->attachedSharedMem = (sharedMemStruct*) shmat( idSharedMem , NULL, 0 );
    }
    return 0;
}

// Método que realiza el hijo para mostrar los resultados leidos desde la memoria compartida
void AdminJust::printStadisticsFromFile( const char* alphchars, const short* wordsPerChar, const size_t amountOfLetters )
{
    // se utiliza un indice secundario para mostrar en orden de letra las palbras que se encuentren en memoria
    // con forme se vayan teniendo
    int subIndex = 0;
    for ( size_t index = 0; index < amountOfLetters; ++ index )
    {
        // se espera a aque el proceso padre de el visto bueno para mostrar los valores
        this->semProcesses.Wait();
        std::cout<<"Palabras que empiezan con la letra: "<<alphchars [ index ]<< std::endl;

        for ( int c = subIndex; c < (subIndex + wordsPerChar [ index ]); ++c )
        {
            std::cout<<"Palabra: "<<this->attachedSharedMem->r[ subIndex ].p<<std::endl;
            std::cout<<"\tAparece "<<this->attachedSharedMem->r [ subIndex ].count<<" veces."<<std::endl;
        }
        std::cout<<std::endl<<std::endl;
        subIndex += wordsPerChar [ index ];
    }
    _exit(0);
}

// método que llena el vector en cada iteración con los mensajes que envían los procesos hijos
void AdminJust::fillMessagesVector ( Reserved childMessages[][Stadistics::AMOUNT_OF_WORDS], size_t &childIndex, size_t &recordAllWords,  int& numberOfFiles )
{
    Reserved receivedMessage;
    // primero se llena el vector en donde van a acumularse los mensajes
    for ( int childNumber = 0; childNumber < numberOfFiles; ++ childNumber )
    {
        messageControl.Receive( receivedMessage, childNumber+ 1 );
        childMessages[ childNumber +1 ] [ childIndex ] = receivedMessage;

        // verificamos si ya algún hijo terminó
        for ( int childFinish = 0; childFinish < numberOfFiles; ++ childFinish )
        {
            messageControl.Receive( receivedMessage, ENDTYPEOP , IPC_NOWAIT );

            if ( receivedMessage.endOperations == true )
            {
                std::cout << receivedMessage.p <<std::endl;
                receivedMessage.endOperations = false;
            }
        }
    }

    // se actualizan los indices de las sublistas. Ahora va por el siguiente
    ++childIndex;

    // se actualizan las palabras recorridas
    ++recordAllWords;
}

// método que se ejecuta una vez que estén todas las palabras de una letra
// escribe en la memoria compartida la cantidad de apariciones de cada palabra
// organizandolas por letra de manera alfabética.
void AdminJust::updateSharedMemory(int & lastPointerToWords, Reserved childMessages[][Stadistics::AMOUNT_OF_WORDS], const short wordsPerChar[], size_t & currentLetterIndex, const  int& numberOfFiles)
{
    Reserved receivedMessage;
    // por cada palabra asociada a la letra contamos cuentas veces está por hijo
    for (int index = lastPointerToWords; index < (lastPointerToWords + wordsPerChar [ currentLetterIndex ]); ++index )
    {
        // se limpia la cadena de caracteres de la estructura del mensaje y la
        // varibale de conteo de la misma.
        for (int clean = 0; clean < TAM; ++ clean)
            receivedMessage.p[ clean ] = '\0';

        receivedMessage.count = 0;

        // copio la palabra para enviarla

        strncpy( receivedMessage.p, childMessages [ 1 ] [ index ].p, strlen( childMessages [ 1 ] [ index ].p ) );
        // por cada hijo busco y sumo esa palabra

        for (int subindex = 0; subindex < numberOfFiles; ++ subindex )
        {
            // sumo las veces que esta aparecce
            receivedMessage.count += childMessages [ subindex + 1 ] [ index ].count;
        }

        this->attachedSharedMem->r[this->attachedSharedMem->nCurrentTotals] = receivedMessage;
        ++this->attachedSharedMem->nCurrentTotals;
    }
    // se indica que el hijo tiene derecho a imprimir
    this->semProcesses.Signal();
    // cambiamos la posición en el vector
    lastPointerToWords += wordsPerChar[ currentLetterIndex ];
    ++currentLetterIndex;
}

int AdminJust::run( std::string listOfFiles[], int numberOfFiles ,short identationSize )
{
    for ( int index = 0; index < numberOfFiles; ++ index )
    {
        if ( !fork() )
        {
            this->individualJust.run( index + 1, messageControl, listOfFiles [ index ], listOfFiles [ index ] , identationSize );
            _exit(0);
        }
    }

    // se leen los mensajes enviados por cada hijo
    Reserved childMessages[ numberOfFiles+1 ][ Stadistics::AMOUNT_OF_WORDS ]; // vector que almacena los mensajes de cada tipo de hijo
    size_t amountOfChars = 20; // cantidad de letras del alfabeto
    size_t currentCharIndex = 0; //indica la letra actual del vector de letras que se está verificando
    size_t childIndex = 0; // control de los indices para organizar la sublista de mensajes por hijo
    size_t recordAllWords = 0; // verifica que solamente se recorran todas las palabras una vez
    int lastPointerToWords = 0; // guadar el orden por donde queda en cada subvector de mensajes de los hijos

    const char alphChars[ amountOfChars ] =
    {
        'a','b','c','d','e','f','g','i','l','m','n','o','p','r','s','t','u','v','w','x'
    };

    const short wordsPerChar[ amountOfChars ] =
    {
        /*a*/6, /*b*/4, /*c*/11, /*d*/6, /*e*/5, /*f*/4, /*g*/1, /*i*/3, /*l*/1, /*m*/1, /*n*/6, /*o**/3, /*p*/3, /*r*/3, /*s*/10, /*t*/ 9, /*u*/3, /*v*/3, /*w*/2, /*x*/2
    };

    // se inicia la memoria compartida
    int isValidSH= this->initSharedMem();

    if ( -1 == isValidSH )
    {
        perror( "ADMINJUST::No se pudo enlazar la memoria compartida");
    }
    else
    {
        // necesitamos tener el id del hijo que imprime para esperarlo por si tarda más en
        // imprimir las palabras
        int printerChildId = fork();

        if (! printerChildId  )
        {
            printStadisticsFromFile(  alphChars,  wordsPerChar, amountOfChars );
        }
        else
        {
            for ( int superIndex = 0; superIndex < Stadistics::AMOUNT_OF_WORDS; ++superIndex )
            {
                // se controla que el recivir mensajes solo se de una vez por cada hijo
                // y n veces por cada palabra
                if (recordAllWords < Stadistics::AMOUNT_OF_WORDS )
                {
                    fillMessagesVector( childMessages,  childIndex, recordAllWords, numberOfFiles );
                }

                // se verifica si ya están todas la palabras de la letra actual para enviarlas a
                // imprimir mediante el hijo que esta esperando
                int signalCounterValue = wordsPerChar [ currentCharIndex ] + lastPointerToWords  - 1 ;
                if ( superIndex == signalCounterValue )
                {
                    updateSharedMemory( lastPointerToWords, childMessages,  wordsPerChar,  currentCharIndex , numberOfFiles);
                }
            }
        }

        // esperamos a que el hijo que imprime termine su ejecución
        int x;
        waitpid( printerChildId, &x , 0 );

        // finalmente solo el papá libera la memoria compartida.
        shmdt( this->attachedSharedMem ) ;
        shmctl( this->idSharedMem, IPC_RMID, NULL );
    }
    return 0;
}

#include "Adminjust.h"

AdminJust::AdminJust()
    :idSharedMem(1)
    ,messageControl()
    ,semProcesses()
    ,individualJust()
{

}

AdminJust::~AdminJust()
{

}

// Método que realiza la creación de la memoria compartida para mensajes
int AdminJust::initSharedMem()
{
    this->idSharedMem = shmget( KEY3, sizeof( sharedMemStruct ),0600 | IPC_CREAT );
    if ( -1 ==  this->idSharedMem )
    {
        return this->idSharedMem; // si hay error, retorna -1
    }
    else
    {
        // enlazamos el puntero a esa memoria creada
        this->attachedSharedMem = (sharedMemStruct*) shmat( idSharedMem , NULL, 0 );
    }
    return 0;
}

// Método que realiza el hijo para mostrar los resultados leidos desde la memoria compartida
void AdminJust::printStadisticsFromFile( const char* alphchars, const short* wordsPerChar, const size_t amountOfLetters )
{
    // se utiliza un indice secundario para mostrar en orden de letra las palbras que se encuentren en memoria
    // con forme se vayan teniendo
    int subIndex = 0;
    for ( size_t index = 0; index < amountOfLetters; ++ index )
    {
        // se espera a aque el proceso padre de el visto bueno para mostrar los valores
        this->semProcesses.Wait();
        std::cout<<"Palabras que empiezan con la letra: "<<alphchars [ index ]<< std::endl;

        for ( int c = subIndex; c < (subIndex + wordsPerChar [ index ]); ++c )
        {
            std::cout<<"Palabra: "<<this->attachedSharedMem->r[ subIndex ].p<<std::endl;
            std::cout<<"\tAparece "<<this->attachedSharedMem->r [ subIndex ].count<<" veces."<<std::endl;
        }
        std::cout<<std::endl<<std::endl;
        subIndex += wordsPerChar [ index ];
    }
    _exit(0);
}

// método que llena el vector en cada iteración con los mensajes que envían los procesos hijos
void AdminJust::fillMessagesVector ( Reserved childMessages[][Stadistics::AMOUNT_OF_WORDS], size_t &childIndex, size_t &recordAllWords,  int& numberOfFiles )
{
    Reserved receivedMessage;
    // primero se llena el vector en donde van a acumularse los mensajes
    for ( int childNumber = 0; childNumber < numberOfFiles; ++ childNumber )
    {
        messageControl.Receive( receivedMessage, childNumber+ 1 );
        childMessages[ childNumber +1 ] [ childIndex ] = receivedMessage;

        // verificamos si ya algún hijo terminó
        for ( int childFinish = 0; childFinish < numberOfFiles; ++ childFinish )
        {
            messageControl.Receive( receivedMessage, ENDTYPEOP , IPC_NOWAIT );

            if ( receivedMessage.endOperations == true )
            {
                std::cout << receivedMessage.p <<std::endl;
                receivedMessage.endOperations = false;
            }
        }
    }

    // se actualizan los indices de las sublistas. Ahora va por el siguiente
    ++childIndex;

    // se actualizan las palabras recorridas
    ++recordAllWords;
}

// método que se ejecuta una vez que estén todas las palabras de una letra
// escribe en la memoria compartida la cantidad de apariciones de cada palabra
// organizandolas por letra de manera alfabética.
void AdminJust::updateSharedMemory(int & lastPointerToWords, Reserved childMessages[][Stadistics::AMOUNT_OF_WORDS], const short wordsPerChar[], size_t & currentLetterIndex, const  int& numberOfFiles)
{
    Reserved receivedMessage;
    // por cada palabra asociada a la letra contamos cuentas veces está por hijo
    for (int index = lastPointerToWords; index < (lastPointerToWords + wordsPerChar [ currentLetterIndex ]); ++index )
    {
        // se limpia la cadena de caracteres de la estructura del mensaje y la
        // varibale de conteo de la misma.
        for (int clean = 0; clean < TAM; ++ clean)
            receivedMessage.p[ clean ] = '\0';

        receivedMessage.count = 0;

        // copio la palabra para enviarla

        strncpy( receivedMessage.p, childMessages [ 1 ] [ index ].p, strlen( childMessages [ 1 ] [ index ].p ) );
        // por cada hijo busco y sumo esa palabra

        for (int subindex = 0; subindex < numberOfFiles; ++ subindex )
        {
            // sumo las veces que esta aparecce
            receivedMessage.count += childMessages [ subindex + 1 ] [ index ].count;
        }

        this->attachedSharedMem->r[this->attachedSharedMem->nCurrentTotals] = receivedMessage;
        ++this->attachedSharedMem->nCurrentTotals;
    }
    // se indica que el hijo tiene derecho a imprimir
    this->semProcesses.Signal();
    // cambiamos la posición en el vector
    lastPointerToWords += wordsPerChar[ currentLetterIndex ];
    ++currentLetterIndex;
}

int AdminJust::run( std::string listOfFiles[], int numberOfFiles ,short identationSize )
{
    for ( int index = 0; index < numberOfFiles; ++ index )
    {
        if ( !fork() )
        {
            this->individualJust.run( index + 1, messageControl, listOfFiles [ index ], listOfFiles [ index ] , identationSize );
            _exit(0);
        }
    }

    // se leen los mensajes enviados por cada hijo
    Reserved childMessages[ numberOfFiles+1 ][ Stadistics::AMOUNT_OF_WORDS ]; // vector que almacena los mensajes de cada tipo de hijo
    size_t amountOfChars = 20; // cantidad de letras del alfabeto
    size_t currentCharIndex = 0; //indica la letra actual del vector de letras que se está verificando
    size_t childIndex = 0; // control de los indices para organizar la sublista de mensajes por hijo
    size_t recordAllWords = 0; // verifica que solamente se recorran todas las palabras una vez
    int lastPointerToWords = 0; // guadar el orden por donde queda en cada subvector de mensajes de los hijos

    const char alphChars[ amountOfChars ] =
    {
        'a','b','c','d','e','f','g','i','l','m','n','o','p','r','s','t','u','v','w','x'
    };

    const short wordsPerChar[ amountOfChars ] =
    {
        /*a*/6, /*b*/4, /*c*/11, /*d*/6, /*e*/5, /*f*/4, /*g*/1, /*i*/3, /*l*/1, /*m*/1, /*n*/6, /*o**/3, /*p*/3, /*r*/3, /*s*/10, /*t*/ 9, /*u*/3, /*v*/3, /*w*/2, /*x*/2
    };

    // se inicia la memoria compartida
    int isValidSH= this->initSharedMem();

    if ( -1 == isValidSH )
    {
        perror( "ADMINJUST::No se pudo enlazar la memoria compartida");
    }
    else
    {
        // necesitamos tener el id del hijo que imprime para esperarlo por si tarda más en
        // imprimir las palabras
        int printerChildId = fork();

        if (! printerChildId  )
        {
            printStadisticsFromFile(  alphChars,  wordsPerChar, amountOfChars );
        }
        else
        {
            for ( int superIndex = 0; superIndex < Stadistics::AMOUNT_OF_WORDS; ++superIndex )
            {
                // se controla que el recivir mensajes solo se de una vez por cada hijo
                // y n veces por cada palabra
                if (recordAllWords < Stadistics::AMOUNT_OF_WORDS )
                {
                    fillMessagesVector( childMessages,  childIndex, recordAllWords, numberOfFiles );
                }

                // se verifica si ya están todas la palabras de la letra actual para enviarlas a
                // imprimir mediante el hijo que esta esperando
                int signalCounterValue = wordsPerChar [ currentCharIndex ] + lastPointerToWords  - 1 ;
                if ( superIndex == signalCounterValue )
                {
                    updateSharedMemory( lastPointerToWords, childMessages,  wordsPerChar,  currentCharIndex , numberOfFiles);
                }
            }
        }

        // esperamos a que el hijo que imprime termine su ejecución
        int x;
        waitpid( printerChildId, &x , 0 );

        // finalmente solo el papá libera la memoria compartida.
        shmdt( this->attachedSharedMem ) ;
        shmctl( this->idSharedMem, IPC_RMID, NULL );
    }
    return 0;
}

#include "Adminjust.h"

AdminJust::AdminJust()
    :idSharedMem(1)
    ,messageControl()
    ,semProcesses()
    ,individualJust()
{

}

AdminJust::~AdminJust()
{

}

// Método que realiza la creación de la memoria compartida para mensajes
int AdminJust::initSharedMem()
{
    this->idSharedMem = shmget( KEY3, sizeof( sharedMemStruct ),0600 | IPC_CREAT );
    if ( -1 ==  this->idSharedMem )
    {
        return this->idSharedMem; // si hay error, retorna -1
    }
    else
    {
        // enlazamos el puntero a esa memoria creada
        this->attachedSharedMem = (sharedMemStruct*) shmat( idSharedMem , NULL, 0 );
    }
    return 0;
}

// Método que realiza el hijo para mostrar los resultados leidos desde la memoria compartida
void AdminJust::printStadisticsFromFile( const char* alphchars, const short* wordsPerChar, const size_t amountOfLetters )
{
    // se utiliza un indice secundario para mostrar en orden de letra las palbras que se encuentren en memoria
    // con forme se vayan teniendo
    int subIndex = 0;
    for ( size_t index = 0; index < amountOfLetters; ++ index )
    {
        // se espera a aque el proceso padre de el visto bueno para mostrar los valores
        this->semProcesses.Wait();
        std::cout<<"Palabras que empiezan con la letra: "<<alphchars [ index ]<< std::endl;

        for ( int c = subIndex; c < (subIndex + wordsPerChar [ index ]); ++c )
        {
            std::cout<<"Palabra: "<<this->attachedSharedMem->r[ subIndex ].p<<std::endl;
            std::cout<<"\tAparece "<<this->attachedSharedMem->r [ subIndex ].count<<" veces."<<std::endl;
        }
        std::cout<<std::endl<<std::endl;
        subIndex += wordsPerChar [ index ];
    }
    _exit(0);
}

// método que llena el vector en cada iteración con los mensajes que envían los procesos hijos
void AdminJust::fillMessagesVector ( Reserved childMessages[][Stadistics::AMOUNT_OF_WORDS], size_t &childIndex, size_t &recordAllWords,  int& numberOfFiles )
{
    Reserved receivedMessage;
    // primero se llena el vector en donde van a acumularse los mensajes
    for ( int childNumber = 0; childNumber < numberOfFiles; ++ childNumber )
    {
        messageControl.Receive( receivedMessage, childNumber+ 1 );
        childMessages[ childNumber +1 ] [ childIndex ] = receivedMessage;

        // verificamos si ya algún hijo terminó
        for ( int childFinish = 0; childFinish < numberOfFiles; ++ childFinish )
        {
            messageControl.Receive( receivedMessage, ENDTYPEOP , IPC_NOWAIT );

            if ( receivedMessage.endOperations == true )
            {
                std::cout << receivedMessage.p <<std::endl;
                receivedMessage.endOperations = false;
            }
        }
    }

    // se actualizan los indices de las sublistas. Ahora va por el siguiente
    ++childIndex;

    // se actualizan las palabras recorridas
    ++recordAllWords;
}

// método que se ejecuta una vez que estén todas las palabras de una letra
// escribe en la memoria compartida la cantidad de apariciones de cada palabra
// organizandolas por letra de manera alfabética.
void AdminJust::updateSharedMemory(int & lastPointerToWords, Reserved childMessages[][Stadistics::AMOUNT_OF_WORDS], const short wordsPerChar[], size_t & currentLetterIndex, const  int& numberOfFiles)
{
    Reserved receivedMessage;
    // por cada palabra asociada a la letra contamos cuentas veces está por hijo
    for (int index = lastPointerToWords; index < (lastPointerToWords + wordsPerChar [ currentLetterIndex ]); ++index )
    {
        // se limpia la cadena de caracteres de la estructura del mensaje y la
        // varibale de conteo de la misma.
        for (int clean = 0; clean < TAM; ++ clean)
            receivedMessage.p[ clean ] = '\0';

        receivedMessage.count = 0;

        // copio la palabra para enviarla

        strncpy( receivedMessage.p, childMessages [ 1 ] [ index ].p, strlen( childMessages [ 1 ] [ index ].p ) );
        // por cada hijo busco y sumo esa palabra

        for (int subindex = 0; subindex < numberOfFiles; ++ subindex )
        {
            // sumo las veces que esta aparecce
            receivedMessage.count += childMessages [ subindex + 1 ] [ index ].count;
        }

        this->attachedSharedMem->r[this->attachedSharedMem->nCurrentTotals] = receivedMessage;
        ++this->attachedSharedMem->nCurrentTotals;
    }
    // se indica que el hijo tiene derecho a imprimir
    this->semProcesses.Signal();
    // cambiamos la posición en el vector
    lastPointerToWords += wordsPerChar[ currentLetterIndex ];
    ++currentLetterIndex;
}

int AdminJust::run( std::string listOfFiles[], int numberOfFiles ,short identationSize )
{
    for ( int index = 0; index < numberOfFiles; ++ index )
    {
        if ( !fork() )
        {
            this->individualJust.run( index + 1, messageControl, listOfFiles [ index ], listOfFiles [ index ] , identationSize );
            _exit(0);
        }
    }

    // se leen los mensajes enviados por cada hijo
    Reserved childMessages[ numberOfFiles+1 ][ Stadistics::AMOUNT_OF_WORDS ]; // vector que almacena los mensajes de cada tipo de hijo
    size_t amountOfChars = 20; // cantidad de letras del alfabeto
    size_t currentCharIndex = 0; //indica la letra actual del vector de letras que se está verificando
    size_t childIndex = 0; // control de los indices para organizar la sublista de mensajes por hijo
    size_t recordAllWords = 0; // verifica que solamente se recorran todas las palabras una vez
    int lastPointerToWords = 0; // guadar el orden por donde queda en cada subvector de mensajes de los hijos

    const char alphChars[ amountOfChars ] =
    {
        'a','b','c','d','e','f','g','i','l','m','n','o','p','r','s','t','u','v','w','x'
    };

    const short wordsPerChar[ amountOfChars ] =
    {
        /*a*/6, /*b*/4, /*c*/11, /*d*/6, /*e*/5, /*f*/4, /*g*/1, /*i*/3, /*l*/1, /*m*/1, /*n*/6, /*o**/3, /*p*/3, /*r*/3, /*s*/10, /*t*/ 9, /*u*/3, /*v*/3, /*w*/2, /*x*/2
    };

    // se inicia la memoria compartida
    int isValidSH= this->initSharedMem();

    if ( -1 == isValidSH )
    {
        perror( "ADMINJUST::No se pudo enlazar la memoria compartida");
    }
    else
    {
        // necesitamos tener el id del hijo que imprime para esperarlo por si tarda más en
        // imprimir las palabras
        int printerChildId = fork();

        if (! printerChildId  )
        {
            printStadisticsFromFile(  alphChars,  wordsPerChar, amountOfChars );
        }
        else
        {
            for ( int superIndex = 0; superIndex < Stadistics::AMOUNT_OF_WORDS; ++superIndex )
            {
                // se controla que el recivir mensajes solo se de una vez por cada hijo
                // y n veces por cada palabra
                if (recordAllWords < Stadistics::AMOUNT_OF_WORDS )
                {
                    fillMessagesVector( childMessages,  childIndex, recordAllWords, numberOfFiles );
                }

                // se verifica si ya están todas la palabras de la letra actual para enviarlas a
                // imprimir mediante el hijo que esta esperando
                int signalCounterValue = wordsPerChar [ currentCharIndex ] + lastPointerToWords  - 1 ;
                if ( superIndex == signalCounterValue )
                {
                    updateSharedMemory( lastPointerToWords, childMessages,  wordsPerChar,  currentCharIndex , numberOfFiles);
                }
            }
        }

        // esperamos a que el hijo que imprime termine su ejecución
        int x;
        waitpid( printerChildId, &x , 0 );

        // finalmente solo el papá libera la memoria compartida.
        shmdt( this->attachedSharedMem ) ;
        shmctl( this->idSharedMem, IPC_RMID, NULL );
    }
    return 0;
}



