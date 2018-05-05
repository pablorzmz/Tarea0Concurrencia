#include <iostream>
#include <vector>

#include "Adminjust.h"

using namespace std;

const std::string help =
        "--help                                     Shows commands information for execution\n"
        "\n"
        "[-e n] input_file_1 input_file_2 input_file_N\n"
        "                                           When command -e is specified, atribute n (most be positive) is waited. If it is skiped,\n"
        "                                           default value of 4 is used. Then, the program reads the list of Code file's names to justify.";

int analizeArgs( int argc, char *argv[] );

int main( int argc, char *argv[] )
{
    return analizeArgs( argc , argv );
}

int analizeArgs(int argc, char *argv[] )
{
    AdminJust admJust; // objeto que realiza las tareas de esta segunda versión con varios procesos
    std::string param;
    std::vector<std::string> lisfOFiles; // lista de archivos leídos que se envía por parámetros
    int identationSize = 4; // tamaño de identación por defecto.

    for (int currentParam = 1; currentParam < argc; ++ currentParam )
    {
        param = argv [currentParam ];
        // muestra la variable ayuda en pantalla y termina la ejecución.
        if ( param == "--help" )
        {
            std::cout<<help<<std::endl;
            exit(1);
        }else
        {
            if ( param == "-e" )
            {
                ++ currentParam;
                if (currentParam < argc )
                {
                    param =  argv [ currentParam ];
                    // se convierte la identación a positivo
                    identationSize = std::stoi( param );

                    if ( identationSize < 0 )
                    {
                        std::cerr<<"Se espera que el tamaño de identación sea positivo y entero";
                        exit(1);
                    }
                }
                else
                {
                    std::cerr<<"Se espera el tamaño de identación luego del comando -e\n";
                    exit(1);
                }
            }else
            {
                // se agregan los archivos a un vector de strings para enviar por parámetros
                lisfOFiles.push_back( param );
            }
        }
    }

    return admJust.run( lisfOFiles, lisfOFiles.size(), identationSize );
}

