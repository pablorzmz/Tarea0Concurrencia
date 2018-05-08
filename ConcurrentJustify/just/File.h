#ifndef FILE_H
#define FILE_H

#define CARRY_RETURN '\r'

#include <fstream>
#include <vector>
#include <iostream>

class File
{
public:
    File();
    int readFile (std::vector<std::string>& lines, const std::string& filenaName );
    void writeFile (const  std::vector<std::string>& lines, const std::string &newFileName, const std::string &extention = "");
    static bool fileExists(const std::string& fileName);
};

#endif // FILE_H
