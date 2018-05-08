#include "Justify.h"

Justify::Justify()
    :utilities()
    ,identationCount( 0 )
    ,identationSize( DEFAULT_IDENTATION )
    ,readLinesFromFile()
    ,codeFile()
    ,st()
{

}

Justify::~Justify()
{

}

int Justify::run(const long& typeX , Buzon &buz, const std::string &codeFileName, const std::string &justifiedFileName , const int &identation )
{    
    // se establece la identación según la indicada por el argumento
    this->identationSize = identation;
    // se leer del archivo existente, el código a identar
    int result = codeFile.readFile( this->readLinesFromFile, codeFileName );
    if ( -1 != result )
    {
        // primero se realiza la separación de la instrucciones por el caracter ;
        this->expand( ';',true );
        // seguidamente, se realiza lo mismo con las llaves { } que corresponden a los bloques
        this->expand( OPENED_KEY );
        this->expand( CLOSED_KEY );
        // finalmente, antes de identar, se eliminan los espacios y tabuladores al principio de cada línea
        this->trimBegin( this->readLinesFromFile );
        // por último, se realiza la identación
        this->ident();
        // antes de finalizar se contruyen la estadisticas de la frecuencia de la palabras reservadas
        st.generateStadistics( buz, typeX, this->readLinesFromFile );
        // finalmente, se crear o reemplaza el nuevo archivo.
        codeFile.writeFile( this->readLinesFromFile, justifiedFileName, EXTENTION );
    }

    return 0;
}

void Justify::trimBegin( std::vector<std::string>& vect)
{
    // método que realiza la eliminación de los espacios al principio de cada línea
    // leída, ya sean espacios blancos o tabuladores, utilizando el vector de strings para
    // iterar.
    size_t linesCount = vect.size();
    for( size_t index = 0; index < linesCount; ++index )
    {
        if ( !utilities.starWithCommentSyntax(vect.at( index ) ) )
        {
            vect.at( index ).erase(0, vect.at( index ).find_first_not_of( SPACE_CHAR ) );
            vect.at( index ).erase(0, vect.at( index ).find_first_not_of( TAB_CHAR ) );
        }
    }
}


void Justify::ident()
{
    // se pide el tamaño del vector una vez para iterar
    size_t linesFromFile = this->readLinesFromFile.size();
    // string para verificar obtenter una cadena limpia sin espacios
    std::string beginChar = "";

    for( size_t index = 0; index < linesFromFile; ++index )
    {
        // si la línea no es un comentario
        if ( !utilities.starWithCommentSyntax(this->readLinesFromFile.at( index ) ) )
        {
            beginChar = this->readLinesFromFile.at( index ).erase(0, this->readLinesFromFile.at( index ).find_first_not_of( SPACE_CHAR ));
            beginChar = this->readLinesFromFile.at( index ).erase(0, this->readLinesFromFile.at( index ).find_first_not_of( TAB_CHAR ));

            // si la línea no está vacía
            if ( !beginChar.empty() )
            {
                // si el caracter corresponde al cierre de bloque } y la identación aún es positiva
                // disminuye el identationCount
                if( beginChar.at(0) == CLOSED_KEY )
                {
                    if ( identationCount > 0 )
                    {
                        --identationCount;
                    }
                }
            }
            // Se agregan los espacios al prnicio correspondientes para cada línea.
            for( short counter = 0; counter < identationCount*identationSize;++counter)
            {
                if ( this->readLinesFromFile.at( index ).empty()==false )
                    this->readLinesFromFile.at( index ).insert( 0, " " );
            }

            // si el caracter corresponde al apertura de bloque {
            // aumenta el identationCount
            if ( !beginChar.empty() )
            {
                if( beginChar.at(0) == OPENED_KEY)
                {
                    ++identationCount;
                }
            }
        }
    }
}


void Justify::expand(const char& charParam, const bool &endLineChar)
{
    size_t linesFromFile = this->readLinesFromFile.size();
    short find = -1;
    short currentPos = -1;
    short findDoubleComma1 = 0;
    short findDoubleComma2 = 0;

    for( size_t index = 0; index < linesFromFile; ++index )
    {
        // Primero se verifica que no sea un comentario
        if( utilities.starWithCommentSyntax( this->readLinesFromFile.at (index ) )== false )
        {
            // se ubica la posición en donde está el caracter.
            find = this->readLinesFromFile.at(index).find_first_of( charParam, find+1 );
            while ( ( size_t ) find != std::string::npos )
            {
                // verifico si estoy en el caso de que sea un char declarado con el caracter del parámetros
                if ( !( ( find-1 > 0 )? ( ( this->readLinesFromFile.at( index ).at( find-1 ) ) == '\'') : false ) )
                {
                    // veo si es alguna cadena de carateres que lo contiene
                    currentPos = -1;
                    findDoubleComma2 = 0;
                    findDoubleComma1  = this->readLinesFromFile.at( index ).find_first_of( '"',currentPos+1 );

                    // veo si hay comillas dobles involucradas.
                    if ( ( size_t )findDoubleComma1 != std::string::npos )
                    {
                        // si hay comilla doble, debería de haber otra para encerrar
                        // el string o la cadena de caracteres, entonces la ubico
                        findDoubleComma2 = this->readLinesFromFile.at( index ).find_first_of( '"', findDoubleComma1+1 );

                        // mientras que hayan llaves metidas en un string, ir descartandolas
                        while ( find > findDoubleComma1 && find < findDoubleComma2 )
                        {
                            find =  this->readLinesFromFile.at( index ).find_first_of( charParam, find+1 );
                        }

                        // aqui puedo haber salido porque es -1 (no hay más)
                        // o también, no hay llaves entre strings enredados.
                        if ( ( size_t )find != std::string::npos )
                        {
                            headAndTailIdentifier( linesFromFile, charParam, index, find, endLineChar );
                        }
                        // se verifica que no sea llaves en comentarios finales
                        // o bien, metidos en el codigo de esta forma /*{}{}{}}{*/
                    }else if( !utilities.containsCommentSyntaxAtEnd( this->readLinesFromFile.at( index ), find ) )
                    {
                        //significa que no hay comentarios involucrados que puedan
                        // interpretarse como llaves o el fin de instrucción
                        // y ahora sí es momento de expandir la llaves (parametro encontrado)
                        if ( ( size_t )find != std::string::npos )
                        {
                            headAndTailIdentifier( linesFromFile, charParam, index, find, endLineChar );
                        }
                    }
                }
                // cambio al siguiente caracter parametro a buscar en el string
                if ( ( size_t )find !=  std::string::npos )
                    find = this->readLinesFromFile.at( index ).find_first_of( charParam, find+1 );
            }
        }
    }
}

// método que realiza la separación de los contenidos de los caracteres de bloque y de fin de instrucción
// de acuerdo a las excepciones de comentarios y ciclo for para el fin de instrucción.
void Justify::headAndTailIdentifier( size_t& linesFromFile, const char &charParam, const size_t& index, const short& find, const bool& endLineChar )
{
    std::string head = "";
    std::string tail = "";
    std::vector<std::string>::iterator iter;

    head = this->readLinesFromFile.at( index ).substr( 0, find );
    tail = this->readLinesFromFile.at( index ).substr( find+1 );

    // se crea un vector temporal para hacer un trim de los espacio al inicio de la cola y la cabeza
    // para, a nivel local de procedimiento, eliminar la identación innecesaria
    std::vector<std::string> trimBeginVect;
    trimBeginVect.push_back( head );
    trimBeginVect.push_back( tail );
    this->trimBegin( trimBeginVect );

    head = trimBeginVect.at( 0 );
    tail = trimBeginVect.at( 1 );
    std::string fromCharToString;
    fromCharToString = charParam;

    if ( endLineChar ) // caso en que sea el caracterer utilizado como fin de instrucción.
    {
        if ( !head.empty() )
        {
            // Exception for sintax instruction
            size_t forIndex = head.find( "for" );
            bool forCondition = false;

            if( forIndex != std::string::npos ) // si no es un ciclo for donde hay caracter ";"
            {
                // se verifica que la presencia de ese for realmente contenga dicho ";" y no sea una
                // una busqueda donde el for se encontraba en otra instrucción,
                size_t parenthesis1 = this->readLinesFromFile.at( index ).find_first_of( OPENED_PARENTHESIS,forIndex );
                size_t parenthesis2 = this->readLinesFromFile.at( index ).find_first_of( CLOSED_PARENTHESIS,parenthesis1 );
                // finalmente si se encuentra contenido en los parentesis de la declaración del for
                if(parenthesis1 != std::string::npos && parenthesis2 != std::string::npos )
                {
                    if ( ( size_t )find > parenthesis1 && ( size_t )find < parenthesis2 )
                    {
                        forCondition = true; // significa que hay un ciclo for que contiene el caracter de fin de instrucción
                    }
                }
            }
            // si no es el caso del for, las instrucciones se separan en donde se haya encontrado el ;
            // siempre y cuando que no sea un comentario para no moverlo de la posición en donde está
            if ( !forCondition )
            {
                head.append( fromCharToString );
                this->readLinesFromFile.at( index ) = head;
                // se sigue respetando los comentarios al final o incrustados en el código
                // entonces estos permanecen intactos.
                if ( !tail.empty() )
                {
                    if ( !utilities.starWithCommentSyntax( tail ) )
                    {
                        moveIteratorToPosition( iter,index );
                        this->readLinesFromFile.insert ( iter, tail );
                        linesFromFile = this->readLinesFromFile.size();
                    }else
                    {
                        this->readLinesFromFile.at( index ).append( tail );
                    }
                }
            }
        }
    }else
    {
        // son operadores como el de la llave { o }
        // verifica que la cabeza no este vacía
        if( !head.empty() )
        {
            // verifica que la cabeza no sea el caracter de bloque actual
            if( head != fromCharToString ) // caso en que el caracter de bloque está solo
            {
                this->readLinesFromFile.at( index ) = head;
                moveIteratorToPosition( iter, index ); // se mueve el iterador para insertar separación
                this->readLinesFromFile.insert( iter, fromCharToString );
                linesFromFile = this->readLinesFromFile.size(); // se actualiza el tamaño
            }
            if ( !tail.empty()) // si la cola no estaba vacía se evalúa lo mismo
            {
                if ( !utilities.starWithCommentSyntax( tail ) )
                {
                    moveIteratorToPosition( iter, index+1 ); // nuevamente se itera para insertar
                    this->readLinesFromFile.insert( iter, tail );
                    linesFromFile = this->readLinesFromFile.size();
                }else
                {
                    iter->append( tail ); // lo comentarios que sean "cola" se mantienen en donde están
                                          //  en cuanto a identación.
                }
            }
        }
        // si no hay cabeza, se verifica que haya cola, pero que tampoco sea comentario
        else if( !tail.empty() && !utilities.starWithCommentSyntax( tail ) )
        {
            moveIteratorToPosition( iter,index );
            this->readLinesFromFile.at( index ) = fromCharToString; // el valor actual del string va a ser el caracter del parametro
            this->readLinesFromFile.insert( iter,tail ); // se inserta el contenido de la cola
            linesFromFile = this->readLinesFromFile.size(); // se actualiza el tamaño de las líneas del vector
        }
    }
}


void Justify::moveIteratorToPosition( std::vector<std::string>::iterator& iter, const size_t &index )
{
    // se mueve el iterador en el vector para insertar la expansión de head-param_caracter-tail
    // en el vector de las líneas leídas.
    iter = this->readLinesFromFile.begin();
    for( size_t counter = 0; counter < index; ++counter )
        ++iter;
    ++iter;
}
