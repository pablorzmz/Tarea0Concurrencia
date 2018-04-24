#include "Util.h"

Util::Util()
{

}

// método que verifica si el archivo contiene al incio, sintaxis de comentario
bool Util::starWithCommentSyntax( const std::string &line )
{
    std::string specialWords[] = {"//","/*","#","/**",";"};
    size_t size = 5;
    std::string temp = line;
    // se itera buscando que si contiene algún coincidencia con el vector specialWords
    for( size_t index = 0; index < size;++index)
    {
        temp.erase( 0, temp.find_first_not_of( ' ' ) ); // se quitan espacios, si los hubiera
        temp.erase( 0, temp.find_first_not_of( '\t' ) ); // se quitan tabuladores, si los hubiera
        if( temp.substr( 0, specialWords[ index ].size() )==specialWords[ index ] )
        {
            return true;  // en caso de coincidir se retorna verdadero
        }
    }
    return false; // por lo contrario, no es un comentario de una línea.
}

// Este método, un poco más complejo que el anterior, evalúa que no haya comentarios de bloque
// incrustados en cualquier parte del código, siempre y cuando, sean de una línea.
// Y que estos NO contengan caracteres de bloque o de fin de instrucción,

bool Util::containsCommentSyntaxAtEnd( const std::string &line, short& find )
{
    size_t commentType1 = line.find_first_of( "//" );
    size_t commentType2 = 0;
                                // caso en que sea un comentario final
    if( commentType1 != std::string::npos)
    {
        if( ( size_t )find > commentType1 )
        {
            return true;
        }else
        {
            // el ciclo verifica que el
            std::string openMultilineComents[] = { "/*","/**","/**" };
            std::string closeMultilineComents[] = { "*/","**/","*/" };
            size_t commentIndex = 3;
            // caso en que sea comentario que dan continuidad al código
            // y que contenga los caracteres de bloque o de fin de instrucción
            // se retorna falso para no realizar acciones de indentación o separación
            for( size_t counter = 0; counter < commentIndex; ++counter )
            {
                // Se busca la prencia de cualquiera de la formas de comentario.
                // Se  sume que el código tiene sentido, consistencia y compila;
                // por lo que se busca el caracter deliminador.
                commentType1 = line.find_first_of( openMultilineComents[ counter ]) ;
                commentType2 = line.find_first_of( closeMultilineComents[ counter ] );
                if( commentType1 != std::string::npos && commentType2 != std::string::npos )
                {
                    // Si el caracter está delimitado por la sintaxis de bloque de código de una línea
                    if ( ( size_t )find > commentType1 && ( size_t ) find < commentType2 )
                    {
                        return true;
                    }
                }
            }
        }
    }
    // De lo contrario, retorna falso. No hay casos definidos de la manera mencionada anteriormente.
    return false;
}
