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
            std::cout<<"Aparece "<<this->attachedSharedMem->r [ subIndex ].count<<" veces."<<std::endl;
        }
        std::cout<<std::endl<<std::endl;
        subIndex += wordsPerChar [ index ];
    }
    _exit(0);
}

// método que llena el vector en cada iteración con los mensajes que envían los procesos hijos
void AdminJust::fillMessagesVector ( Reserve childMessages[][Stadistics::AMOUNT_OF_WORDS], size_t &childIndex, size_t &recordAllWords,  int& numberOfFiles )
{
    Reserve receivedMessage;
    // primero se llena el vector en donde van a acumularse los mensajes
    for ( int childNumber = 0; childNumber < numberOfFiles; ++ childNumber )
    {
        messageControl.Receive( receivedMessage, childNumber+ 1 );
        childMessages[ childNumber +1 ] [ childIndex ] = receivedMessage;
    }

    // se actualizan los indices de las sublistas. Ahora va por el siguiente
    ++childIndex;

    // se actualizan las palabras recorridas
    ++recordAllWords;
}

// método que se ejecuta una vez que estén todas las palabras de una letra
// escribe en la memoria compartida la cantidad de apariciones de cada palabra
// organizandolas por letra de manera alfabética.
void AdminJust::updateSharedMemory(int & lastPointerToWords, Reserve childMessages[][Stadistics::AMOUNT_OF_WORDS], const short wordsPerChar[], size_t & currentLetterIndex, const  int& numberOfFiles)
{
    Reserve receivedMessage;
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
    Reserve childMessages[ numberOfFiles+1 ][ Stadistics::AMOUNT_OF_WORDS ]; // vector que almacena los mensajes de cada tipo de hijo
    size_t amountOfLetters = 20; // cantidad de letras del alfabeto
    size_t currentLetterIndex = 0; //indica la letra actual del vector de letras que se está verificando
    size_t childIndex = 0; // control de los indices para organizar la sublista de mensajes por hijo
    size_t recordAllWords = 0; // verifica que solamente se recorran todas las palabras una vez
    int lastPointerToWords = 0; // guadar el orden por donde queda en cada subvector de mensajes de los hijos

    const char alphChars[amountOfLetters] =
    {
        'a','b','c','d','e','f','g','i','l','m','n','o','p','r','s','t','u','v','w','x'
    };

    const short wordsPerChar[amountOfLetters] =
    {
        /*a*/6, /*b*/4, /*c*/11, /*d*/6, /*e*/5, /*f*/4, /*g*/1, /*i*/3, /*l*/1, /*m*/1, /*n*/6, /*o**/3, /*p*/3, /*r*/3, /*s*/10, /*t*/ 9, /*u*/3, /*v*/3, /*w*/2, /*x*/2
    };

    // para revisar el conteo final de la palabra actual
    short tempCount[ numberOfFiles +1 ]; // almacena la cuenta total de palabras de la misma letra

    short partialCount = 0;

    for ( int index = 0; index < numberOfFiles +1; ++ index )
        tempCount [ index ] = 0;

    this->initSharedMem();
    int printerChildId = fork();
    if (! printerChildId  )
    {
        printStadisticsFromFile(  alphChars,  wordsPerChar, amountOfLetters );
    }
    else
    {
        for ( short amountOfWords = 0; amountOfWords < Stadistics::AMOUNT_OF_WORDS; ++amountOfWords )
        {
            // se controla que el recivir mensajes solo se de una vez por cada hijo
            // y n veces por cada palabra
            if (recordAllWords < Stadistics::AMOUNT_OF_WORDS )
            {
                fillMessagesVector( childMessages,  childIndex, recordAllWords, numberOfFiles );
            }

            // se verifica si ya están todas las palabras con la letra especifica por parte de ese hijo
            for ( int childNumberX = 0; childNumberX < numberOfFiles; ++ childNumberX )
            {
                for (int subVectorIndex = 0; subVectorIndex < Stadistics::AMOUNT_OF_WORDS; ++ subVectorIndex)
                {
                    if ( childMessages [ childNumberX + 1 ][ subVectorIndex ].p[0] == alphChars [ currentLetterIndex ]  )
                    {
                        tempCount [ childNumberX + 1 ] += 1;
                    }
                }
            }

            // ahora se suma a ver si ya si tienen el registro de todas las palabras de la letra actual
            for ( int childNumberY = 0; childNumberY < numberOfFiles; ++ childNumberY )
                partialCount += tempCount [ childNumberY + 1 ];

            // ahora se evalua si la suma es igual a la cantidad de palabras de dicha letra mutiplicado por la cantidad de hijos
            if ( partialCount == (wordsPerChar [ currentLetterIndex ] * numberOfFiles) )
            {

                updateSharedMemory( lastPointerToWords, childMessages,  wordsPerChar,  currentLetterIndex, numberOfFiles);

            }else
            {
                // si no, se reinicia la cuenta
                for ( short index = 0; index < numberOfFiles +1; ++ index )
                {
                    tempCount [ index + 1  ] = 0;
                }
                partialCount = 0;
            }
            // si ya termine todas los mesajes de los hijos, pero
            // aún no termino de contar si ya estan todas las palabras
            if ( amountOfWords ==  (Stadistics::AMOUNT_OF_WORDS -1 ) && currentLetterIndex != amountOfLetters  )
                --amountOfWords;
        }
    }

    // esperamos a que el hijo que imprime termine su ejecución
    int x;
    waitpid( printerChildId, &x , 0 );

    // finalmente solo el papá libera la memoria compartida.
    shmdt( this->attachedSharedMem ) ;
    shmctl( this->idSharedMem, IPC_RMID, NULL );

    return 0;
}
