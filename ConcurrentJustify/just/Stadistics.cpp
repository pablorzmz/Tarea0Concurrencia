#include "Stadistics.h"

#define PALABRAS_RESERVADAS "reserved_words.txt"
Stadistics::Stadistics()
    :stats()
    ,reservedWordsVector()
    ,finder()
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
    for (size_t index = 0; index < this->reservedWordsVector.size(); ++index )
    {
        finder.insert(std::pair<std::string,int>(reservedWordsVector[index],0) );
    }


}


void Stadistics::generateStadistics(  Buzon & buz, const long& typeX, const std::vector<std::string>& lines , const std::string &originalFileName )
{    
    Reserv mensajeLocal;

    size_t lineFromFile = lines.size();

    size_t amountChars = 9;
    size_t startPos = 0;

    std::string sintaxChars[amountChars] = {";",":", "&", "(", ")",",","<",">","-"};

    std::string copiedLine = "";
    std::string wordTokens = "";
    Util utilities;

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
                std::map<std::string, short>::iterator iter =  finder.find(wordTokens);

            }
        }
    }

}
