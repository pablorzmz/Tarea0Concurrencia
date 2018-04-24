#ifndef UTIL_H
#define UTIL_H

#include <string>

class Util
{
public:
    Util();
    bool containsCommentSyntaxAtEnd(const std::string& line, short &find);
    bool starWithCommentSyntax(const std::string& line);
};

#endif // UTIL_H
