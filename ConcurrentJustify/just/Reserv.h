#ifndef RESERV_H
#define RESERV_H

#define TAM 32
#define VECTORSIZE 100

class Reserved
{

public:
    Reserved();
    int p;
    short count;
    bool endOperations;
    bool isAnErrorMessage;
};

#endif // RESERV_H
