#ifndef STADISTICS_H
#define STADISTICS_H

#define ENDTYPEOP 703

#include <ctype.h>
#include <vector>
#include <string>
#include <map>
#include <sstream>
#include "Buzon.h"
#include "Util.h"
#include "Reserv.h"
#include <algorithm>

class Stadistics
{        
    std::map<std::string,short> finder;
 public:
    const static short AMOUNT_OF_WORDS = 86;
    static const std::string reservedWordsVector[ Stadistics::AMOUNT_OF_WORDS ];

    Stadistics();
    ~Stadistics();
    void fillWords();
    void generateStadistics(Buzon &buz, const long &typeX, const std::vector<std::string>& lines );
};

#endif // STADISTICS_H
