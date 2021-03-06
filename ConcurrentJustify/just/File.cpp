#include "File.h"

File::File()
{

}

// método que realiza la lectura de todo el contenido de un archivo dado. Cada línea, hasta el final del archivo
// se inserta en un vector de strings dado para luego ser procesado.

int File::readFile( std::vector<std::string> &lines, const std::string &filenaName)
{
    std::string readLine;
    std::ifstream codeFile;

    if ( filenaName.empty() ) // si el nombre del archivo es vacío se asume la entrada estandar
    {
        while( std::getline( std::cin, readLine ) )
        {
            lines.push_back( readLine );
        }
    }else // por otro lado, se intenta abrir el archivo para leer su contenido
    {
        codeFile.open( filenaName , std::ios::in);
        if( codeFile.is_open() )
        {
            while( std::getline( codeFile, readLine ) )
            {
                if ( !readLine.empty() && readLine.at( readLine.size() -1 ) == CARRY_RETURN )
                    readLine.erase( readLine.size() -1 );
                lines.push_back( readLine );
            }
            codeFile.close();
        }else // en caso de no poder abrir, se notifica y finaliza la ejecución.
        {
            std::cerr<<"No se pudo abrir el archivo: "<<filenaName<<std::endl;
            return -1;
        }
    }
    return 0;
}

// método genérico para escibir los string del vector en un archivo válido dado por el argumento newFileName

void File::writeFile( const std::vector<std::string> &lines,const  std::string &newFileName,  const std::string &extention )
{
    size_t counter = lines.size();
    std::string fileWithExt = newFileName;

    if ( fileWithExt.empty() ) // si el archivo para escribir es vacío se asume la salida estándar
    {
        for(size_t index = 0; index < counter; ++index)
            std::cout<<lines.at( index )<<std::endl;
    }else // de lo contrario, al nombre se le añade la extensión y se escribe cada línea del vector de strings.
    {
        fileWithExt.append(extention);
        std::ofstream newFile;
        newFile.open ( fileWithExt, std::ios::out );
        if ( newFile.is_open() )
        {
            for ( size_t index = 0; index < counter; ++index )
            {
                newFile<<lines.at( index )<<std::endl;
            }
            newFile.close();
        }else // si por alguna razón no se pudo abrir, se notifica la excepción y se finaliza la ejecución.
        {
            std::cerr<<"No se pudo crear/abrir el archivo: "<<fileWithExt<<std::endl;
            exit(1);
        }
    }
}

// métdo que evalúa si el archivo exsite o no
bool File::fileExists(const std::string &fileName)
{
   std::ifstream file;
   file.open( fileName );
   if ( file.is_open() ) // si se puede abrir existe y retorna verdadero
   {
       file.close();
       return true;
   }else
   {
       return false; // de lo contrario, es falso.
   }
}
