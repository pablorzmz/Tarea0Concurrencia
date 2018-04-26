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

    for ( int index = 0; index < numberOfFiles; ++ index )
    {
        if ( !fork() )
        {
            justIndividual.run( index + 1, messageControl, listOfFiles [ index ], listOfFiles [ index ] , identationSize );
            _exit(0);
        }
    }

    // se leen los mensajes enviados por cada hijo
    Reserv childMessages[ numberOfFiles+1 ][ Stadistics::AMOUNT_OF_WORDS ]; // vector que almacena los mensajes de cada tipo de hijo
    Reserv receivedMessage; // variable para obtener el mensaje
    size_t amountOfLetters = 26; // cantidad de letras del alfabeto
    size_t currentLetterIndex = 0; //indica la letra actual del vector de letras que se está verificando
    size_t childIndex = 0; // control de los indices para organizar la sublista de mensajes por hijo
    size_t recordAllWords = 0; // verifica que solamente se recorran todas las palabras una vez

    const char letter[amountOfLetters] =
    {
        'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z'
    };

    const short wordsPerLetter[amountOfLetters] =
    {
        /*a*/6, /*b*/4, /*c*/11, /*d*/6, /*e*/5, /*f*/4, /*g*/1, /*h*/0, /*i*/3, /*j*/0, /*k*/0, /*l*/1, /*m*/1, /*n*/6, /*o*/3, /*p*/3, /*q*/0, /*r*/3, /*s*/10, /*t*/ 9, /*u*/3, /*v*/3, /*w*/2, /*x*/2, /*y*/0, /*z*/0
    };

    // para revisar el conteo final de la palabra actual
    short tempCount[ numberOfFiles +1 ]; // almacena la cuenta total de palabras de la misma letra

    short partialCount = 0;

    for ( int index = 0; index < numberOfFiles +1; ++ index )
        tempCount [ index ] = 0;

    for ( short amountOfWords = 0; amountOfWords < Stadistics::AMOUNT_OF_WORDS; ++amountOfWords )
    {
        // se controla que el recivir mensajes solo se de una vez por cada hijo
        // y n veces por cada palabra
        if (recordAllWords < Stadistics::AMOUNT_OF_WORDS )
        {
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

        // se verifica si ya están todas las palabras con la letra especifica por parte de ese hijo
        for ( int childNumberX = 0; childNumberX < numberOfFiles; ++ childNumberX )
        {
            for (int subVectorIndex = 0; subVectorIndex < Stadistics::AMOUNT_OF_WORDS; ++ subVectorIndex)
            {
                if ( childMessages [ childNumberX + 1 ][ subVectorIndex ].p[0] == letter [ currentLetterIndex ]  )
                {
                    tempCount [ childNumberX + 1 ] += 1;
                }
            }
        }

        // ahora se suma a ver si ya si tienen el registro de todas las palabras de la letra actual
        for ( int childNumberY = 0; childNumberY < numberOfFiles; ++ childNumberY )
            partialCount += tempCount [ childNumberY + 1 ];

        // ahora se evalua si la suma es igual a la cantidad de palabras de dicha letra mutiplicado por la cantidad de hijos
        if ( partialCount == (wordsPerLetter [ currentLetterIndex ] * numberOfFiles) )
        {
            // si es así, se cuenta cuantas veces aparece cada palabra
            std::cout <<"Tengo todas las palabras que comienzan con "<< letter [ currentLetterIndex ]<<std::endl;
            std::cout <<"Son en total: "<< ( partialCount ) << std::endl;
            std::cout <<"Hay: "<< ( partialCount / numberOfFiles ) <<" por cada hijo"<< std::endl<<std::endl;
            ++currentLetterIndex;

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
    return 0;
}
