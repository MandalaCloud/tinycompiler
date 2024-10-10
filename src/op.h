#include "ast.h"

class Op : public BaseAST {
    public:
    std::unique_ptr<BaseAST> op;
};

class LT : public BaseAST {
    public:
    std::string op;
};

class LE : public BaseAST {
    public:
    std::string op;
};

class GT : public BaseAST {
    public:
    std::string op;
};

class GE : public BaseAST {
    public:
    std::string op;
};


class EQ : public BaseAST {
    public:
    std::string op;
};

class NE : public BaseAST {
    public:
    std::string op;
};

class AND : public BaseAST {
    public:
    std::string op;
};

class OR : public BaseAST {
    public:
    std::string op;
};