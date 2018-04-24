#include "Stadistics.h"

#define PALABRAS_RESERVADAS "reserved_words.txt"
Stadistics::Stadistics()
    :stats()
    ,reservedWordsVector()
    ,stadisticsResult()
{
    this->fillWords();
}

Stadistics::~Stadistics()
{

}

void Stadistics::fillWords()
{
    File reader;
    std::string fileName =  PALABRAS_RESERVADAS;
    reader.readFile( this->reservedWordsVector, fileName );
}


void Stadistics::generateStadistics(  Buzon & buz, const long& typeX, const std::vector<std::string>& lines , const std::string &originalFileName )
{    
    Reserv mensajeLocal;
    strncpy( mensajeLocal.p, "Ya terminé", TAM );
    mensajeLocal.c =typeX;

    size_t reservedWordsSize = this->reservedWordsVector.size();
    size_t lineFromFile = lines.size();
    short counter[ reservedWordsSize ];

    size_t amountChars = 9;
    size_t startPos = 0;

    std::string sintaxChars[amountChars] = {";",":", "&", "(", ")",",","<",">","-"};

    std::string copiedLine = "";
    std::string wordTokens = "";
    Util utilities;


    //Llenamos el arry dinamico con las palabras
    for ( size_t index = 0; index < reservedWordsSize; ++index )
    {
        counter[index] = 0;
    }

    // contamos cuantas palabras reservadas hay
    for ( size_t index = 0; index < lineFromFile;++index )
    {
        if ( !utilities.starWithCommentSyntax( lines.at( index ) ) )
        {
            // hacemos una copia de la línea leída
            copiedLine = lines.at(index);
            // se tranforma en minúscula
            std::transform( copiedLine.begin(), copiedLine.end(), copiedLine.begin(), tolower );
            // separamos las palabras reservadas que pudieron quedar "pegadas a otros caracteres"
            for ( size_t subIndex = 0; subIndex < amountChars; ++subIndex )
            {
                startPos = copiedLine.find_first_of( sintaxChars [subIndex] );
                while ( startPos != std::string::npos )
                {
                    copiedLine.replace( startPos, sintaxChars[ subIndex ].length()," "+sintaxChars[ subIndex ]+" ");
                    startPos = copiedLine.find_first_of( sintaxChars [subIndex],startPos + 2 );
                }
            }
            // se crea un tokenizador para separar por espacios en blanco
            std :: istringstream tokens (copiedLine);
            while ( tokens >> wordTokens )
            {
                for ( size_t subIndex2 = 0; subIndex2 < reservedWordsSize; ++subIndex2 )
                {
                    if ( wordTokens == reservedWordsVector.at( subIndex2 ) )
                    {
                        ++counter [ subIndex2 ];
                        buz.Enviar( mensajeLocal, typeX );
                    }
                }
            }
        }
    }

}
