#include <iostream>
#include "Adminjust.h"

using namespace std;

const std::string help =
"--help                                     Shows commands information for execution\n"
"\n"
"[-e n] [-i input_file] [-o output_file]        When command -e is specified, atribute n (most be positive) is waited. If it is skiped,\n"
"                                           default value of 4 is used. Command -i instroduces input_file name, and it is waited. When\n"
"                                           this command is skiped, standard input is used and the name for the stadistics´s file will\n"
"                                           be standarInput.Command -o instroduces output_file name, and it is waited. When this command\n"
"                                           is skiped, standard output is used. Also the program runs with changed input (<) and ouput\n"
"                                           files(>).\n";

int analizeArgs( int argc, char *argv[] );

int main( int argc, char *argv[] )
{

    return analizeArgs( argc , argv );
}

int analizeArgs(int argc, char *argv[] )
{
    AdminJust admJust;
    short size = 3;
    std::string s[size];
    int identationSize = 0;

    s[0] = "solution1.cpp";
    s[1] = "solution2.cpp";
    s[2] = "solution3.cpp";

    return admJust.run( s, size, identationSize );
}

