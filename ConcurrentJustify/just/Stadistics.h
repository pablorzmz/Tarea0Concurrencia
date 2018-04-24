#ifndef STADISTICS_H
#define STADISTICS_H

#include <ctype.h>
#include <vector>
#include <string>
#include "File.h"
#include <sstream>
#include "Buzon.h"
#include "Util.h"
#include "Reserv.h"
#include <algorithm>

class Stadistics
{        
private:
    File stats;

public:
    std::vector<std::string> reservedWordsVector;
    std::vector<std::string> stadisticsResult;

    Stadistics();
    ~Stadistics();
    void fillWords();
    void generateStadistics(Buzon &buz, const long &typeX, const std::vector<std::string>& lines , const std::string &originalFileName);
};

#endif // STADISTICS_H
