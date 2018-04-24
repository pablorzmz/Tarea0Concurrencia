#ifndef JUSTIFY_H
#define JUSTIFY_H

#define DEFAULT_NAME "STANDARD_INPUT"
#define OPENED_KEY '{'
#define CLOSED_KEY '}'
#define OPENED_PARENTHESIS '('
#define CLOSED_PARENTHESIS ')'
#define EXTENTION ".sgr"
#define DEFAULT_IDENTATION 4
#define SPACE_CHAR 32
#define TAB_CHAR '\t'

#include <algorithm>
#include <iostream>
#include <vector>
#include "Buzon.h"
#include "File.h"
#include "Stadistics.h"
#include "Util.h"


class Justify
{
public:
    Justify();
    ~Justify();
    int run(const long& typeX , Buzon &buz, const std::string &codeFileName, const std::string &justifiedFileName , const int &identation = DEFAULT_IDENTATION);

private:
    Util utilities;
    short identationCount;
    short identationSize;
    std::vector<std::string> readLinesFromFile;
    File codeFile;
    Stadistics st;    

    void trimBegin(std::vector<std::string> &vect);
    void ident();
    void expand(const char &charParam, const bool& endLineChar = false);   
    void headAndTailIdentifier(size_t& linesFromFile, const char &charParam, const size_t& index, const short& find, const bool &endLineChar);
    void moveIteratorToPosition(std::vector<std::string>::iterator &iter, const size_t& index);


};

#endif // JUSTIFY_H
