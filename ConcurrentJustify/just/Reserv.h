#ifndef RESERV_H
#define RESERV_H

#define TAM 32
#define VECTORSIZE 100

class Reserved
{

public:
    Reserved();
    char p[TAM];
    short count;
    bool endOperations;
    bool isAnErrorMessage;
};

#endif // RESERV_H
