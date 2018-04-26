#ifndef RESERV_H
#define RESERV_H

#define TAM 32

class Reserv
{
public:
    Reserv();
    char p[TAM];
    short count;
    bool failOperations;
    bool endOperations;
};

#endif // RESERV_H
