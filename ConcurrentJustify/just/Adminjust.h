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
        Reserve r[ Stadistics::AMOUNT_OF_WORDS ];
        sharedMemStruct()
            :nCurrentTotals(0)
            ,r()
        {

        }
    };
public:
    AdminJust();
    ~AdminJust();
    int run(std::string listOfFiles[], int numberOfFiles, short identationSize);
    int idSharedMem;
private:
    Buzon messageControl;
    Semaforo semProcesses;    
    sharedMemStruct* attachedSharedMem;
    Justify individualJust;
    int initSharedMem();
    void printStadisticsFromFile(const char* alphchars, const short* wordsPerChar, const size_t amountOfLetters );
    void fillMessagesVector(Reserve childMessages[][Stadistics::AMOUNT_OF_WORDS], size_t& childIndex, size_t& recordAllWords,  int& numberOfFiles );
    void updateSharedMemory(int & lastPointerToWords, Reserve childMessages[][Stadistics::AMOUNT_OF_WORDS], const short wordsPerChar[], size_t & currentLetterIndex, const  int& numberOfFiles);

};

#endif // ADMINJUST_H
