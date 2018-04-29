#include <iostream>
#include <vector>

#include "Adminjust.h"

using namespace std;

const std::string help =
"--help                                     Shows commands information for execution\n"
"\n"
"[-e n] input_file_1 input_file_2 input_file_N "
"                                           "
"                                           When command -e is specified, atribute n (most be positive) is waited. If it is skiped,\n"
"                                           default value of 4 is used. Then, the program reads the list of Code file's names to justify.";

int analizeArgs( int argc, char *argv[] );

int main( int argc, char *argv[] )
{
    return analizeArgs( argc , argv );
}

int analizeArgs(int argc, char *argv[] )
{
    AdminJust admJust;
    std::string param;
    std::vector<std::string> lisfOFiles;
    int identationSize = 4;

    for (int currentParam = 1; currentParam < argc; ++ currentParam )
    {
        param = argv [currentParam ];
        if ( param == "-e" )
        {
            ++ currentParam;
            if (currentParam < argc )
            {
                param =  argv [ currentParam ];
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
            lisfOFiles.push_back( param );
        }
    }

    return admJust.run( lisfOFiles, lisfOFiles.size(), identationSize );
}

