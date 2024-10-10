#ifndef AST_H
#define AST_H

#include <iostream>
#include <memory>
#include <string>
#include <string.h>
#include <cstring>
#include <vector>
#include <variant>
#include <map>
#include <algorithm>

extern int expNumCnt, symTabCnt, allsymTabCnt, ifCnt, brctNumCnt, whileNumCnt;
extern std::string now_while_end, now_while_entry,strtmp,globaltype;
extern std::map<std::string, std::pair<int, int>> symbol_table;
extern std::map<std::string, std::pair<int, int>> *current_table;
extern std::map<std::string, std::variant<int, std::string>> *curFunvar_table;
extern std::map<std::map<std::string, std::pair<int, int>>*, std::map<std::string, std::pair<int, int>>*> total_table;
extern std::map<std::string, std::string> func_table;
extern std::map<std::string, std::pair<int, int>> glob_table;

extern std::map<std::string, int> *cur_array_dims_table;
extern std::map<std::map<std::string, int>*, std::map<std::string, int>*> total_array_dims_table;

class BaseAST; 
class CompUnitAST;
class FuncDefAST;
class FuncTypeAST;
class BlockAST;
class BlockItemAST;
class DeclAST;
class ConstDeclAST;
class BTypeAST;
class ConstDefAST;
class ConstInitValAST;
class VarDeclAST;
class VarDefAST;
class InitValAST;
class LValAST;
class ConstExpAST;
class StmtAST;
class ExpAST;
class PrimaryExpAST;
class UnaryExpAST;
class UnaryOpAST;
class MulExpAST;
class AddExpAST;
class RelExpAST;
class EqExpAST;
class LAndAST;
class LOrAST;
class NumberAST;
class FuncFParamAST;
class FuncFParamsAST;
class FuncRParamsAST;
class CompUnitListAST;
class CompUnitListItemAST;
class ArrayIndexConstExpList;
class ArrayIndexExpList;

class BaseAST {
 public:
  int val;
  BaseAST() = default;
  virtual ~BaseAST() = default;

};


class CompUnitAST : public BaseAST {
 public:
  std::unique_ptr<CompUnitListAST> list;

  void Dump(std::string& str0) const;
};

class CompUnitListAST : public BaseAST {
 public:
  std::vector<std::unique_ptr<CompUnitListItemAST>> items;

  void Dump(std::string& str0) const;
};

class CompUnitListItemAST : public BaseAST {
 public:
  std::unique_ptr<FuncDefAST> funcdef;
  std::unique_ptr<DeclAST> decl;
  enum TAG {DECL, FUNCDEF};
  TAG tag;

  void Dump(std::string& str0) const;
};

class FuncDefAST : public BaseAST {
 public:
  std::string type;
  std::string ident;
  std::unique_ptr<BlockAST> block;
  std::unique_ptr<FuncFParamsAST> params;
  enum TAG {NONE, PARAMS};
  TAG tag;

  void Dump(std::string& str0) const;
};

class FuncTypeAST : public BaseAST {
 public:
  std::string type;

  void Dump(std::string& str0) const;
};

class FuncFParamsAST : public BaseAST {
 public:
  std::vector<std::unique_ptr<FuncFParamAST>> params;

  void Dump(std::string& str0) const;
};

class FuncFParamAST : public BaseAST {
 public:
  std::string ident;
  std::vector<std::unique_ptr<ConstExpAST>> constexps;
  enum TAG {IDENT, EMPTY, ARRAY};
  TAG tag;

  std::string typeForArrayFParam(int& totalDimsNum);
  std::vector<int> const_value();
  void Dump(std::string& str0);
};

class BlockAST : public BaseAST {
 public:
  std::vector<std::unique_ptr<BlockItemAST>> blockitems;
  std::string type;
  bool isret ;

  void Dump(std::string& str0) ;
};

class BlockItemAST : public BaseAST{
 public:
  std::unique_ptr<StmtAST> stmt;
  std::unique_ptr<DeclAST> decl;
  enum TAG {DECL, STMT};
  TAG tag;

  void Dump(std::string& str0) const;
};

class DeclAST: public BaseAST{
 public:
  std::unique_ptr<ConstDeclAST> constdecl;
  std::unique_ptr<VarDeclAST> vardecl;
  enum TAG {CONST, VAL};
  TAG tag;

  void Dump(std::string& str0,bool ifglobal=false) const;
};

class ConstDeclAST: public BaseAST{
 public:
  std::vector<std::unique_ptr<ConstDefAST>> constdefs;
  std::unique_ptr<BTypeAST> btype;

  void Dump(std::string& str0,bool ifglobal) const;
};

class BTypeAST: public BaseAST{
 public:
  std::string btype ;
};

class ConstDefAST: public BaseAST{
 public:
  std::string ident;
  std::unique_ptr<ConstInitValAST> constinitval;
  enum TAG {ARRAY, EQ};
  TAG tag; 
  std::vector<std::unique_ptr<ConstExpAST>> constexps;

  void globalInitSearch(std::vector<int>& dims, std::vector<int>& numsForDims, int curDimIndex, std::string& str0, std::vector<std::string>& initArray, int initValIndex);
  void getElemPtr(std::string ptrIdent,  std::vector<int>& dims, std::vector<int>& numsForDims, int curPtrIndex, std::string& str0, std::vector<std::string>& initArray, int initValIndex);
  void Dump(std::string& str0,bool ifglobal);
  std::vector<int> const_value();
};

class ConstInitValAST: public BaseAST{
 public:
  std::unique_ptr<ConstExpAST> constexp;
  std::vector<std::unique_ptr<ConstInitValAST>> inits;
  enum TAG {EXP, INIT, EMPTY, INITARRAY};
  TAG tag; 

  void getInitValArray(std::vector<std::string>& resVec, std::vector<int>& numsForDims, int brace, int align, std::string& str0);
  std::string DumpStr(std::string& str0);
};

class VarDeclAST: public BaseAST{
 public:
  std::vector<std::unique_ptr<VarDefAST>> vardefs;

  void Dump(std::string& str0,bool ifglobal) const;
};

class VarDefAST: public BaseAST{
 public:
  std::string ident;
  std::unique_ptr<InitValAST> initval;
  std::vector<std::unique_ptr<ConstExpAST>> constexps;
  enum TAG {IDENT, EQ, IDENTARRAY, EQARRAY};
  TAG tag;

  std::vector<int> const_value();
  void Dump(std::string& str0,bool ifglobal);
  void getElemPtr(std::string ptrIdent,  std::vector<int>& dims, std::vector<int>& numsForDims, int curPtrIndex, std::string& str0, std::vector<std::string>& initArray, int initValIndex);
  void globalInitSearch(std::vector<int>& dims, std::vector<int>& numsForDims, int curDimIndex, std::string& str0, std::vector<std::string>& initArray, int initValIndex);
};

class InitValAST: public BaseAST{
 public:
  std::unique_ptr<ExpAST> exp;
  std::vector<std::unique_ptr<InitValAST>> inits;
  enum TAG {EXP, INIT, EMPTY, INITARRAY};
  TAG tag; 
  void getInitValArray(std::vector<std::string>& resVec, std::vector<int>& numsForDims, int brace, int align, std::string& str0);
  std::string DumpStr(std::string& str0);
};

class LValAST: public BaseAST{
 public:
  std::string ident;
  std::vector<std::unique_ptr<ExpAST>> exps;
  enum TAG {ARRAY, IDENT};
  TAG tag;

  std::string getElemPtrForLVal(std::string& str0, std::string& resident, bool isArrayFunParam);
  std::string DumpStr(std::string& str0);
};

class ConstExpAST: public BaseAST{
  public:
  std::unique_ptr<ExpAST> exp;

  std::string DumpStr(std::string& str0);
};

class StmtAST : public BaseAST {
 public:
  std::unique_ptr<ExpAST> exp;
  std::unique_ptr<LValAST> lval;
  std::unique_ptr<BlockAST> block;
  std::unique_ptr<StmtAST> stmt1;
  std::unique_ptr<StmtAST> stmt2;
  std::string type;
  enum TAG {RET_EXP, RET, NONE, BLOCK, EXP, EQ, IF, IFELSE, WHILE, BREAK, CONTINUE};
  TAG tag;
  int val;
  bool isret = false ;

  void Dump(std::string& str0) ;
};

class ExpAST : public BaseAST {
 public:
  std::unique_ptr<LOrAST> lorexp;

  std::string calculate(std::string& str0);
  std::string DumpStr(std::string& str0);
};

class PrimaryExpAST : public BaseAST {
 public:
  enum TAG {EXP, NUM , LVAL};
  TAG tag;
  std::unique_ptr<ExpAST> exp;
  std::unique_ptr<NumberAST> number;
  std::unique_ptr<LValAST> lval;
  int val;

  std::string DumpStr(std::string& str0);
  std::string calculate(std::string& str0);
};

class UnaryOpAST : public BaseAST {
 public:
  std::string op;
};

class UnaryExpAST : public BaseAST {
 public:
  enum TAG {EXP, OP, NONE, PARAMS};
  TAG tag;
  std::unique_ptr<PrimaryExpAST> primaryexp;
  char unaryop;
  std::unique_ptr<UnaryExpAST> unaryexp;
  int val;
  std::string ident;
  std::unique_ptr<FuncRParamsAST> params;

  std::string DumpStr(std::string& str0);
  std::string calculate(std::string& str0);
};

class FuncRParamsAST : public BaseAST {
 public:
  std::vector<std::unique_ptr<ExpAST>> exps;

  void Dump(std::string& str0) const ;
};

class MulExpAST : public BaseAST {
 public:
  enum TAG {EXP, MUL, DIV, MOD};
  TAG tag;
  std::unique_ptr<UnaryExpAST> unaryexp;
  std::unique_ptr<MulExpAST> mulexp;
  int val;

  std::string DumpStr(std::string& str0);
  std::string calculate(std::string& str0);
};

class AddExpAST : public BaseAST {
 public:
  enum TAG {EXP, ADD, SUB};
  TAG tag;
  std::unique_ptr<MulExpAST> mulexp;
  std::unique_ptr<AddExpAST> addexp1;
  std::unique_ptr<MulExpAST> mulexp1;
  int val;

  std::string DumpStr(std::string& str0);
  std::string calculate(std::string& str0);
};

class RelExpAST : public BaseAST {
 public:
  enum TAG {EXP, LT, GE, LE, GT};
  TAG tag;
  std::unique_ptr<AddExpAST> addexp;
  std::unique_ptr<RelExpAST> relexp;
  int val;

  std::string DumpStr(std::string& str0);
  std::string calculate(std::string& str0);
};

class EqExpAST : public BaseAST {
 public:
  enum TAG {EXP, EQ, NE};
  TAG tag;
  std::unique_ptr<RelExpAST> relexp;
  std::unique_ptr<EqExpAST> eqexp;
  int val;

  std::string DumpStr(std::string& str0);
  std::string calculate(std::string& str0);
};

class LAndAST : public BaseAST {
 public:
  enum TAG {EXP, AND};
  TAG tag;
  std::unique_ptr<EqExpAST> eqexp;
  std::unique_ptr<LAndAST> landexp;
  int val;

  std::string DumpStr(std::string& str0);
  std::string calculate(std::string& str0);
};

class LOrAST : public BaseAST {
 public:
  enum TAG {EXP, OR};
  TAG tag;
  std::unique_ptr<LAndAST> landexp;
  std::unique_ptr<LOrAST> lorexp;
  int val;

  std::string DumpStr(std::string& str0);
  std::string calculate(std::string& str0);
};

class NumberAST : public BaseAST {
 public:
  int val;

  std::string DumpStr(std::string& str0) ;
};

class ArrayIndexConstExpList : public BaseAST {
public:
    std::vector<std::unique_ptr<ConstExpAST>> constexps;
};

class ArrayIndexExpList : public BaseAST {
public:
    std::vector<std::unique_ptr<ExpAST>> exps;
};

#endif