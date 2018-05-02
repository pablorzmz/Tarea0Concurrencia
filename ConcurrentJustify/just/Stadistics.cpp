#include "Stadistics.h"


const std::string Stadistics::reservedWordsVector[ Stadistics::AMOUNT_OF_WORDS ] = {
        "alignas",
        "alignof",
        "and",
        "and_eq",
        "asm",
        "auto",
        "bitand",
        "bitor",
        "bool",
        "break",
        "case",
        "catch",
        "char",
        "char16_t",
        "char32_t",
        "class",
        "compl",
        "const",
        "constexpr",
        "const_cast",
        "continue",
        "decltype",
        "default",
        "delete",
        "do",
        "double",
        "dynamic_cast",
        "else",
        "enum",
        "explicit",
        "export",
        "extern",
        "false",
        "float",
        "for",
        "friend",
        "goto",
        "if",
        "inline",
        "int",
        "long",
        "mutable",
        "namespace",
        "new",
        "noexcept",
        "not",
        "not_eq",
        "nullptr",
        "operator",
        "or",
        "or_eq",
        "private",
        "protected",
        "public",
        "register",
        "reinterpret_cast",
        "return",
        "short",
        "signed",
        "sizeof",
        "size_t",
        "static",
        "static_assert",
        "static_cast",
        "std",
        "struct",
        "switch",
        "template",
        "this",
        "thread_local",
        "throw",
        "true",
        "try",
        "typedef",
        "typeid",
        "typename",
        "union",
        "unsigned",
        "using",
        "virtual",
        "void",
        "volatile",
        "wchar_t",
        "while",
        "xor",
        "xor_eq",
    };

Stadistics::Stadistics()
    :finder()    

{
    this->fillWords();
}

Stadistics::~Stadistics()
{    
}


void Stadistics::fillWords()
{

    for (size_t index = 0; index <  AMOUNT_OF_WORDS; ++index )
    {
        finder.insert(std::pair<std::string,int>( reservedWordsVector[index],0) );
    }

}


void Stadistics::generateStadistics(  Buzon & buz, const long& typeX, const std::vector<std::string>& lines )
{    
    Reserved localMessage;

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
                startPos = copiedLine.find_first_of( sintaxChars [ subIndex ] );
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
                // se busca la palabra y aumenta el contador
                std::map<std::string, short>::iterator iter =  finder.find( wordTokens );
                if ( iter != finder.end() )
                    iter->second +=1;
            }
        }
    }

    // se envian los mensajes por cada palabra
    int index = 0;
    for ( std::map<std::string,short>::iterator iter = finder.begin(); iter != finder.end(); ++iter )
    {
        localMessage.p = index;
        localMessage.count = iter-> second;
        buz.Send( localMessage, typeX );
        ++index;
    }
}
