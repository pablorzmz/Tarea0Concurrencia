#ifndef ADMINJUST_H
#define ADMINJUST_H

#define KEY3 0xB65700


#include "Buzon.h"
#include "Justify.h"
#include "Semaforo.h"
#include "Reserv.h"
#include <string.h>
#include <sys/ipc.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <unistd.h>

class AdminJust
{
    struct sharedMemStruct
    {
        int nCurrentTotals;
        Reserved r[ Stadistics::AMOUNT_OF_WORDS ];
        sharedMemStruct()
            :nCurrentTotals(0)
            ,r()
        {

        }
    };
public:
    AdminJust();
    ~AdminJust();
    int run(std::vector<std::string> listOfFiles, int numberOfFiles, size_t identationSize);
    int idSharedMem;
private:
    Buzon messageControl;
    Semaforo semProcesses;    
    sharedMemStruct* attachedSharedMem;
    Justify individualJust;
    int initSharedMem();
    void printStadisticsFromFile(const char* alphchars, const short* wordsPerChar, const size_t amountOfLetters );
    void fillMessagesVector(Reserved childMessages[][Stadistics::AMOUNT_OF_WORDS], size_t& childIndex, size_t& recordAllWords,  int& numberOfFiles );
    void updateSharedMemory(int & lastPointerToWords, Reserved childMessages[][Stadistics::AMOUNT_OF_WORDS], const short wordsPerChar[], size_t & currentLetterIndex, const  int& numberOfFiles);

};

#endif // ADMINJUST_H
