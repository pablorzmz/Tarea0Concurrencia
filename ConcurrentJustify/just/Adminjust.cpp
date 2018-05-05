#include "Adminjust.h"

AdminJust::AdminJust()
    :idSharedMem(1)
    ,messageControl()
    ,semProcessesChild(0, 0xA12345)
    ,semProcessesFather(0, 0xA12346)
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
void AdminJust::printStadisticsFromFile( const char* alphchars, const size_t amountOfLetters )
{
    // se utiliza un indice secundario para mostrar en orden de letra las palbras que se encuentren en memoria
    // con forme se vayan teniendo
    //int subIndex = 0;
    for ( size_t index = 0; index < amountOfLetters; ++ index )
    {
        // se espera a aque el proceso padre de el visto bueno para mostrar los valores
        this->semProcessesChild.Wait();
        std::cout<<"Palabras que empiezan con la letra: "<<alphchars [ index ]<< std::endl;

        //for ( int c = subIndex; c < (subIndex + wordsPerChar [ index ]); ++c )
        for ( int c = 0; c < this->attachedSharedMem->nCurrentTotals; ++c )
        {
            std::cout<<"Palabra: "<<Stadistics::reservedWordsVector[ this->attachedSharedMem->r[ c ].p ]<<std::endl;
            std::cout<<"\tAparece "<<this->attachedSharedMem->r [ c ].count<<" veces."<<std::endl;
        }
        std::cout<<std::endl<<std::endl;
        // regresa el control al padre para que siga sumando
        this->semProcessesFather.Signal();
    }
    _exit(0);
}

// método que llena el vector en cada iteración con los mensajes que envían los procesos hijos
void AdminJust::fillMessagesVector ( Reserved childMessages[][Stadistics::AMOUNT_OF_WORDS], size_t &childIndex, int& numberOfFiles )
{
    Reserved receivedMessage;
    // primero se llena el vector en donde van a acumularse los mensajes
    for ( int childNumber = 0; childNumber < numberOfFiles; ++ childNumber )
    {
        messageControl.Receive( receivedMessage, childNumber+ 1 );
        childMessages[ childNumber +1 ] [ childIndex ] = receivedMessage;
    } 

    // se actualizan los indices de las sublistas. Ahora va por el siguiente
    ++childIndex;   
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

        receivedMessage.count = 0;

        // copio el indice la palabra para enviarla de cualquier hijo
        receivedMessage.p = childMessages[ 1 ] [index ].p;

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
    this->semProcessesChild.Signal();
    // espero a que el hijo me indique que puedo seguir
    this->semProcessesFather.Wait();
    // cambiamos la posición en el vector
    lastPointerToWords += wordsPerChar[ currentLetterIndex ];
    ++currentLetterIndex;
    // se reinicia el indice de la memoria compartida
    this->attachedSharedMem->nCurrentTotals = 0;
}

int AdminJust::run(std::vector<std::string> listOfFiles, int numberOfFiles , size_t identationSize )
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
            printStadisticsFromFile( alphChars, amountOfChars );
        }
        else
        {
            for ( int superIndex = 0; superIndex < Stadistics::AMOUNT_OF_WORDS; ++superIndex )
            {

                // recibo una palabra por hijo y voy llenando el vector de mensajes
                fillMessagesVector( childMessages,  childIndex, numberOfFiles );

                // se verifica si ya están todas la palabras de la letra actual para enviarlas a
                // imprimir mediante el hijo que esta esperando
                int signalCounterValue = wordsPerChar [ currentCharIndex ] + lastPointerToWords  - 1 ;
                if ( superIndex == signalCounterValue )
                {
                    updateSharedMemory( lastPointerToWords, childMessages,  wordsPerChar,  currentCharIndex , numberOfFiles);
                }
            }
        }
        // finalmente solo el papá libera la memoria compartida.
        shmdt( this->attachedSharedMem ) ;
        shmctl( this->idSharedMem, IPC_RMID, NULL );
    }
    return 0;
}
