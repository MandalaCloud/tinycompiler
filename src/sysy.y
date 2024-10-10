%code requires {
  #include <memory>
  #include <string>
}

%{

#include <iostream>
#include <memory>
#include <string>
#include <cstring>
#include <string.h>
#include "op.h"

int yylex();
void yyerror(std::unique_ptr<BaseAST> &ast, const char *s);

using namespace std;

%}

%parse-param { std::unique_ptr<BaseAST> &ast }

%union {
  std::string *str_val;
  int int_val;
  char char_val;
  BaseAST *ast_val;
}

// 终结符
%token INT RETURN IF ELSE WHILE BREAK CONTINUE VOID
%token <str_val> IDENT CONST
%token <int_val> INT_CONST
%token <ast_val> LE GE EQ NE LT GT AND OR OP

// 非终结符
%type <ast_val> FuncDef FuncType Block Stmt MatchedStmt OpenStmt CompUnit Number
%type <ast_val> Exp PrimaryExp UnaryExp AddExp MulExp RelExp EqExp LAndExp LOrExp
%type <ast_val> BlockItemList BlockItem Decl ConstDef ConstDecl ConstDefList BType ConstInitVal VarDecl VarDefList VarDef InitVal LVal ConstExp
%type <ast_val> FuncFParam FuncRParams FuncFParams CompUnitList CompUnitListItem ArrayIndexConstExpList ConstInitValList InitValList ArrayIndexExpList
%type <char_val> UnaryOp

%%

CompUnit
  : CompUnitList {
    auto comp_unit = make_unique<CompUnitAST>();
    comp_unit->list = unique_ptr<CompUnitListAST>((CompUnitListAST*)$1);
    ast = move(comp_unit);
  }
  ;

CompUnitList
  : CompUnitListItem {
    auto ast = new CompUnitListAST();
    ast->items.emplace_back((CompUnitListItemAST*)$1);
    $$ = ast;
  } 
  | CompUnitList CompUnitListItem {
    auto ast = new CompUnitListAST();
    auto list = unique_ptr<CompUnitListAST>((CompUnitListAST*)$1);
    int n = list->items.size();
    for(int i = 0; i < n; ++i){
        ast->items.emplace_back(list->items[i].release());
    }
    ast->items.emplace_back((CompUnitListItemAST*)$2);
    $$ = ast;
  } 
  ;


CompUnitListItem
  : Decl {
    auto ast = new CompUnitListItemAST();
    ast->decl= unique_ptr<DeclAST>((DeclAST *)$1);
    ast->tag = CompUnitListItemAST::DECL;
    $$ = ast;
  }
  | FuncDef {
    auto ast = new CompUnitListItemAST();
    ast->funcdef= unique_ptr<FuncDefAST>((FuncDefAST *)$1);
    ast->tag = CompUnitListItemAST::FUNCDEF;
    $$ = ast;
  } 
  ;

FuncDef
  : INT IDENT '(' ')' Block {
    auto ast = new FuncDefAST();
    ast->tag = FuncDefAST::NONE;
    ast->type = "int";
    ast->ident = *unique_ptr<string>($2);
    ast->block = unique_ptr<BlockAST>((BlockAST*)$5);
    $$ = ast;
  }
  | VOID IDENT '(' ')' Block {
    auto ast = new FuncDefAST();
    ast->tag = FuncDefAST::NONE;
    ast->type = "void";
    ast->ident = *unique_ptr<string>($2);
    ast->block = unique_ptr<BlockAST>((BlockAST*)$5);
    $$ = ast;
  }
  | INT IDENT '(' FuncFParams ')' Block {
    auto ast = new FuncDefAST();
    ast->tag = FuncDefAST::PARAMS;
    ast->type = "int";
    ast->ident = *unique_ptr<string>($2);
    ast->params = unique_ptr<FuncFParamsAST>((FuncFParamsAST*)$4);
    ast->block = unique_ptr<BlockAST>((BlockAST*)$6);
    $$ = ast;
  }
  | VOID IDENT '(' FuncFParams ')' Block {
    auto ast = new FuncDefAST();
    ast->tag = FuncDefAST::PARAMS;
    ast->type = "void";
    ast->ident = *unique_ptr<string>($2);
    ast->params = unique_ptr<FuncFParamsAST>((FuncFParamsAST*)$4);
    ast->block = unique_ptr<BlockAST>((BlockAST*)$6);
    $$ = ast;
  }
  ;

FuncFParams
  : FuncFParam {
    auto func_params = new FuncFParamsAST();
    func_params->params.emplace_back((FuncFParamAST *)$1);
    $$ = func_params;
  } | FuncFParam ',' FuncFParams {
    auto func_params = new FuncFParamsAST();
    func_params->params.emplace_back((FuncFParamAST *)$1);
    auto params2 = unique_ptr<FuncFParamsAST>((FuncFParamsAST *)($3));
    int n = params2->params.size();
    for(int i = 0; i < n; ++i){
        func_params->params.emplace_back(params2->params[i].release());
    }
    $$ = func_params;
  }
  ;

FuncFParam
  : FuncType IDENT {
    auto func_param = new FuncFParamAST();
    func_param->tag = FuncFParamAST::IDENT;
    func_param->ident = *unique_ptr<string>($2);
    $$ = func_param;
  } 
  | FuncType IDENT '[' ']' {
    auto func_param = new FuncFParamAST();
    func_param->tag = FuncFParamAST::EMPTY;
    func_param->ident = *unique_ptr<string>($2);
    $$ = func_param;
  } | FuncType IDENT '[' ']' ArrayIndexConstExpList {
    auto func_param = new FuncFParamAST();
    func_param->tag = FuncFParamAST::ARRAY;
    func_param->ident = *unique_ptr<string>($2);
    unique_ptr<ArrayIndexConstExpList> p((ArrayIndexConstExpList *)$5);
    for(auto &ce : p->constexps){
        func_param->constexps.emplace_back(ce.release());
    }
    $$ = func_param;
  }
  ;

FuncType
  : INT {
    auto ast = new FuncTypeAST();
    std::string str= "int";
    ast->type = str;
    $$ = ast;
  }
  | VOID {
    auto ast = new FuncTypeAST();
    std::string str= "void";
    ast->type = str;
    $$ = ast;
  }
  ;

Block
  : '{' BlockItemList '}' {
    $$ = $2;
  }
  ;

BlockItemList
  : {
    auto block = new BlockAST();
    $$ = block;
  } | BlockItem BlockItemList {
    auto block = new BlockAST();
    auto block_lst = unique_ptr<BlockAST>((BlockAST *)$2);
    block->blockitems.emplace_back((BlockItemAST *)$1);
    int n = block_lst->blockitems.size();
    for(int i = 0; i < n; ++i){
      block->blockitems.emplace_back(block_lst->blockitems[i].release());
    }
    $$ = block;
  }
  ;

BlockItem
  : Decl {
    auto block_item = new BlockItemAST();
    block_item->tag = BlockItemAST::DECL;
    block_item->decl = unique_ptr<DeclAST>((DeclAST *)$1);
    $$ = block_item;
  }
  | Stmt {
    auto block_item = new BlockItemAST();
    block_item->tag = BlockItemAST::STMT;
    block_item->stmt = unique_ptr<StmtAST>((StmtAST *)$1);
    $$ = block_item;
  }
  ;

Decl 
  : ConstDecl {
    auto decl = new DeclAST();
    decl->tag = DeclAST::CONST;
    decl->constdecl = unique_ptr<ConstDeclAST>((ConstDeclAST *)$1);
    $$ = decl;
  }
  | VarDecl {
    auto decl = new DeclAST();
    decl->tag = DeclAST::VAL;
    decl->vardecl = unique_ptr<VarDeclAST>((VarDeclAST *)$1);
    $$ = decl;
  }
  ;

ConstDef
  : IDENT '=' ConstInitVal {
    symbol_table[*$1] = make_pair($3->val, 1);
    auto const_def = new ConstDefAST();
    const_def->tag = ConstDefAST::EQ;
    const_def->ident = *unique_ptr<string>($1);
    const_def->constinitval = unique_ptr<ConstInitValAST>((ConstInitValAST *)$3);
    $$ = const_def;
  }
  ;

ConstDef
  : IDENT ArrayIndexConstExpList '=' ConstInitVal {
    auto const_def = new ConstDefAST();
    unique_ptr<ArrayIndexConstExpList> p( (ArrayIndexConstExpList *)$2);
    const_def->tag = ConstDefAST::ARRAY;
    const_def->ident = *unique_ptr<string>($1);
    for(auto &ce : p->constexps){
        const_def->constexps.emplace_back(ce.release());
    }
    const_def->constinitval = unique_ptr<ConstInitValAST>((ConstInitValAST *)$4);
    $$ = const_def;
  }
  ;

ArrayIndexConstExpList
  : '[' ConstExp ']' {
    auto p = new ArrayIndexConstExpList();
    p->constexps.emplace_back((ConstExpAST *)$2);
    $$ = p;
  } | ArrayIndexConstExpList '[' ConstExp ']' {
    auto p = (ArrayIndexConstExpList *)$1;
    p->constexps.emplace_back((ConstExpAST *)$3);
    $$ = p;
  }
  ;

ConstDecl
  : CONST BType ConstDefList ';'{
    auto const_decl = (ConstDeclAST *)$3;
    const_decl->btype = unique_ptr<BTypeAST>((BTypeAST *)$2);
    $$ = const_decl;
  }
  ;

ConstDefList
  : ConstDef ',' ConstDefList {
    auto const_decl = new ConstDeclAST();
    auto const_decl_2 = unique_ptr<ConstDeclAST>((ConstDeclAST *)$3);
    const_decl->constdefs.emplace_back((ConstDefAST *)$1);
    int n = const_decl_2->constdefs.size();
    for(int i = 0; i < n; ++i){
        const_decl->constdefs.emplace_back(const_decl_2->constdefs[i].release());
    }
    $$ = const_decl;
  }
  | ConstDef {
    auto const_decl = new ConstDeclAST();
    const_decl->constdefs.emplace_back((ConstDefAST *)$1);
    $$ = const_decl;
  }
  ;

BType
  : INT {
    auto ret = new BTypeAST();
    string str = "int" ;
    ret -> btype = str ;
    $$ = ret;
  }
  ;

ConstInitVal
  : ConstExp {
    auto const_init_val = new ConstInitValAST();
    const_init_val->tag = ConstInitValAST::EXP;
    const_init_val->constexp = unique_ptr<ConstExpAST>((ConstExpAST *)$1);
    $$ = const_init_val;
  } |'{' '}' {
    auto const_init_val = new ConstInitValAST();
    const_init_val->tag = ConstInitValAST::EMPTY;
    $$ = const_init_val;
  } | '{' ConstInitValList '}' {
    $$ = $2;
  }
  ;

ConstInitValList
  : ConstInitVal {
    auto init_val = new ConstInitValAST();
    init_val->tag = ConstInitValAST::INIT;
    init_val->inits.emplace_back((ConstInitValAST *)$1);
    $$ = init_val;
  } | ConstInitValList ',' ConstInitVal {
    auto init_val = (ConstInitValAST *)$1;
    init_val->tag = ConstInitValAST::INITARRAY;
    init_val->inits.emplace_back((ConstInitValAST *)$3);
    $$ = init_val;
  }
  ;

VarDecl
  : INT VarDefList ';' {
    auto var_decl = (VarDeclAST *)$2;
    $$ = var_decl;
  }
  ;

VarDefList
  : VarDef ',' VarDefList {
    auto var_decl = new VarDeclAST();
    auto var_decl_2 = unique_ptr<VarDeclAST>((VarDeclAST *)$3);
    var_decl->vardefs.emplace_back((VarDefAST *)$1);
    int n = var_decl_2->vardefs.size();
    for(int i = 0; i < n; ++i){
        var_decl->vardefs.emplace_back(var_decl_2->vardefs[i].release());
    }
    $$ = var_decl;
  }
  | VarDef {
    auto var_decl = new VarDeclAST();
    var_decl->vardefs.emplace_back((VarDefAST *)$1);
    $$ = var_decl;
  }
  ;

VarDef
  : IDENT{
    auto var_def = new VarDefAST();
    var_def->tag = VarDefAST::IDENT;
    var_def->ident = *unique_ptr<string>($1);
    symbol_table[var_def->ident] = make_pair(0x7fffffff, -1);
    $$ = var_def;
  } | IDENT ArrayIndexConstExpList {
    auto var_def = new VarDefAST();
    var_def->tag = VarDefAST::IDENTARRAY;
    var_def->ident = *unique_ptr<string>($1);
    unique_ptr<ArrayIndexConstExpList> p((ArrayIndexConstExpList *)$2);
    for(auto &ce : p->constexps){
        var_def->constexps.emplace_back(ce.release());
    }
    $$ = var_def;
  } | IDENT '=' InitVal {
    auto var_def = new VarDefAST();
    var_def->tag = VarDefAST::EQ;
    var_def->ident = *unique_ptr<string>($1);
    var_def->initval = unique_ptr<InitValAST>((InitValAST *)$3);
    symbol_table[var_def->ident] = make_pair(var_def->initval->exp->val, 0);
    $$ = var_def;
  } | IDENT ArrayIndexConstExpList '=' InitVal {
    auto var_def = new VarDefAST();
    var_def->tag = VarDefAST::EQARRAY;
    var_def->ident = *unique_ptr<string>($1);
    unique_ptr<ArrayIndexConstExpList> p((ArrayIndexConstExpList *)$2);
    for(auto &ce : p->constexps){
        var_def->constexps.emplace_back(ce.release());
    }
    var_def->initval = unique_ptr<InitValAST>((InitValAST *)$4);
    $$ = var_def;
  }
  ;

InitVal
  : Exp{
    auto init_val = new InitValAST();
    init_val->tag = InitValAST::EXP;
    init_val->exp.reset((ExpAST *)$1);
    $$ = init_val;
  } | '{' '}' {
    auto init_val = new InitValAST();
    init_val->tag = InitValAST::EMPTY;
    $$ = init_val;
  } | '{' InitValList '}' {
    $$ = $2;
  }

InitValList
  : InitVal {
    auto init_val = new InitValAST();
    init_val->tag = InitValAST::INIT;
    init_val->inits.emplace_back((InitValAST *)$1);
    $$ = init_val;
  } | InitValList ',' InitVal {
    auto init_val = (InitValAST *)$1;
    init_val->tag = InitValAST::INITARRAY;
    init_val->inits.emplace_back((InitValAST *)$3);
    $$ = init_val;
  }
  ;

LVal
  : IDENT {
    auto lval = new LValAST();
    lval->tag = LValAST::IDENT;
    lval->ident = *unique_ptr<string>($1);
    lval->val = symbol_table[lval->ident].first;
    $$ = lval;
  } | IDENT ArrayIndexExpList {
    auto lval = new LValAST();
    unique_ptr<ArrayIndexExpList> p((ArrayIndexExpList *)$2);
    lval->tag = LValAST::ARRAY;
    lval->ident = *unique_ptr<string>($1);
    for(auto &e : p->exps){
        lval->exps.emplace_back(e.release());
    }
    $$ = lval;
  }
  ;

ArrayIndexExpList
  : '[' Exp ']' {
    auto p = new ArrayIndexExpList();
    p->exps.emplace_back((ExpAST *)$2);
    $$ = p;
  } | ArrayIndexExpList '[' Exp ']' {
    auto p = (ArrayIndexExpList *)$1;
    p->exps.emplace_back((ExpAST *)$3);
    $$ = p;
  }
  ;

ConstExp
  : Exp {
    auto const_exp = new ConstExpAST();
    const_exp->exp = unique_ptr<ExpAST>((ExpAST *)$1);
    $$ = const_exp;
  }
  ;

Stmt
  : MatchedStmt {
    $$ = $1;
  }
  | OpenStmt {
    $$ = $1;
  }

MatchedStmt 
  : RETURN Exp ';' {
    auto ast = new StmtAST();
    ast->tag = StmtAST::RET_EXP;
    ast->val = $2->val;
    ast->exp = unique_ptr<ExpAST>((ExpAST *)$2);
    ast->isret = true;
    $$ = ast;
  }
  | RETURN ';' {
    auto stmt = new StmtAST();
    stmt->tag = StmtAST::RET;
    stmt->isret = true;
    $$ = stmt;
  }
  | LVal '=' Exp ';' {
    auto stmt = new StmtAST();
    stmt->tag = StmtAST::EQ;
    stmt->exp.reset((ExpAST *) $3);
    stmt->lval.reset((LValAST *) $1);
    stmt->val = $3->val;
    stmt->lval->val = $3->val;
    $$ = stmt;
  }
  | Exp ';' {
    auto ast = new StmtAST();
    ast->tag = StmtAST::EXP;
    ast->val = $1->val;
    ast->exp = unique_ptr<ExpAST>((ExpAST *)$1);
    $$ = ast;
  }
  | ';' {
    auto stmt = new StmtAST();
    stmt->tag = StmtAST::NONE;
    $$ = stmt;
  }
  | Block {
    auto ast = new StmtAST();
    ast->tag = StmtAST::BLOCK;
    ast->block = unique_ptr<BlockAST>((BlockAST *)$1);
    $$ = ast;
  }
  | IF '(' Exp ')' MatchedStmt ELSE MatchedStmt {
    auto ast = new StmtAST();
    ast->tag = StmtAST::IFELSE;
    ast->exp = unique_ptr<ExpAST>((ExpAST *)$3);
    ast->stmt1 = unique_ptr<StmtAST>((StmtAST *)$5);
    ast->stmt2 = unique_ptr<StmtAST>((StmtAST *)$7);
    $$ = ast;
  }
  | WHILE '(' Exp ')' Stmt {
    auto ast = new StmtAST();
    ast->tag = StmtAST::WHILE;
    ast->exp = unique_ptr<ExpAST>((ExpAST *)$3);
    ast->stmt1 = unique_ptr<StmtAST>((StmtAST *)$5);
    $$ = ast;
  }
  | BREAK ';' {
    auto stmt = new StmtAST();
    stmt->tag = StmtAST::BREAK;
    $$ = stmt;
  }
  | CONTINUE ';' {
    auto stmt = new StmtAST();
    stmt->tag = StmtAST::CONTINUE;
    $$ = stmt;
  }
  ;

OpenStmt
  : IF '(' Exp ')' Stmt {
    auto ast = new StmtAST();
    ast->tag = StmtAST::IF;
    ast->exp = unique_ptr<ExpAST>((ExpAST *)$3);
    ast->stmt1 = unique_ptr<StmtAST>((StmtAST *)$5);
    $$ = ast;
  }
  | IF '(' Exp ')' MatchedStmt ELSE OpenStmt {
    auto ast = new StmtAST();
    ast->tag = StmtAST::IFELSE;
    ast->exp = unique_ptr<ExpAST>((ExpAST *)$3);
    ast->stmt1 = unique_ptr<StmtAST>((StmtAST *)$5);
    ast->stmt2 = unique_ptr<StmtAST>((StmtAST *)$7);
    $$ = ast;
  }
  ;

Exp
  : LOrExp {
    auto ast = new ExpAST();
    ast->lorexp = unique_ptr<LOrAST>((LOrAST*)$1);
    $$ = ast;
  }
  ;

PrimaryExp
  : '(' Exp ')' {
    auto ast = new PrimaryExpAST();
    ast->exp = unique_ptr<ExpAST>((ExpAST *)$2);
    ast->tag = PrimaryExpAST::EXP;
    $$ = ast;
  }
  | Number {
    auto ast = new PrimaryExpAST();
    ast->number = unique_ptr<NumberAST>((NumberAST *)$1);
    ast->tag = PrimaryExpAST::NUM;
    $$ = ast;
  }
  | LVal {
    auto primary_exp = new PrimaryExpAST();
    primary_exp->tag = PrimaryExpAST::LVAL;
    primary_exp->lval = unique_ptr<LValAST>((LValAST *)$1);
    $$ = primary_exp;
  }
  ;

UnaryExp 
  : PrimaryExp{
    auto ast = new UnaryExpAST();
    ast->primaryexp = unique_ptr<PrimaryExpAST>((PrimaryExpAST *)$1);
    ast->tag = UnaryExpAST::EXP;
    $$ = ast;
  }
  | UnaryOp UnaryExp{
    auto ast = new UnaryExpAST();
    ast->unaryexp = unique_ptr<UnaryExpAST>((UnaryExpAST *)$2);
    ast->unaryop = $1 ;
    ast->tag = UnaryExpAST::OP;
    $$ = ast;
  }
  | IDENT '(' ')' {
    auto unary_exp = new UnaryExpAST();
    unary_exp->tag = UnaryExpAST::NONE;
    unary_exp->ident = *unique_ptr<string>($1);
    $$ = unary_exp;
  } | IDENT '(' FuncRParams ')' {
    auto unary_exp = new UnaryExpAST();
    unary_exp->tag = UnaryExpAST::PARAMS;
    unary_exp->ident = *unique_ptr<string>($1);
    unary_exp->params.reset((FuncRParamsAST *)$3);
    $$ = unary_exp;
  }
  ;

FuncRParams
  : Exp {
    auto ast = new FuncRParamsAST();
    ast->exps.emplace_back((ExpAST *)$1);
    $$ = ast;
  } | Exp ',' FuncRParams {
    auto ast = new FuncRParamsAST();
    ast->exps.emplace_back((ExpAST *)$1);
    auto p2 = unique_ptr<FuncRParamsAST>((FuncRParamsAST *)$3);
    int n = p2->exps.size();
    for(int i = 0; i < n; ++i){
        ast->exps.emplace_back(p2->exps[i].release());
    }
    $$ = ast;
  }
  ;

AddExp
  : MulExp {
    auto ast = new AddExpAST();
    ast->tag = AddExpAST::EXP;
    ast->mulexp = unique_ptr<MulExpAST>((MulExpAST *)$1);
    $$ = ast;
  }
  | AddExp '+' MulExp {
    auto ast = new AddExpAST();
    ast->tag = AddExpAST::ADD;
    ast->addexp1 = unique_ptr<AddExpAST>((AddExpAST *)$1);
    ast->mulexp1 = unique_ptr<MulExpAST>((MulExpAST *)$3);
    $$ = ast;
  }
  | AddExp '-' MulExp {
    auto ast = new AddExpAST();
    ast->tag = AddExpAST::SUB;
    ast->addexp1 = unique_ptr<AddExpAST>((AddExpAST *)$1);
    ast->mulexp1 = unique_ptr<MulExpAST>((MulExpAST *)$3);
    $$ = ast;
  }
  ;

MulExp 
  : UnaryExp {
    auto ast = new MulExpAST();
    ast->tag = MulExpAST::EXP;
    ast->unaryexp = unique_ptr<UnaryExpAST>((UnaryExpAST *)$1);
    $$ = ast;
  }
  | MulExp '*' UnaryExp {
    auto ast = new MulExpAST();
    ast->tag = MulExpAST::MUL;
    ast->mulexp = unique_ptr<MulExpAST>((MulExpAST *)$1);
    ast->unaryexp = unique_ptr<UnaryExpAST>((UnaryExpAST *)$3);
    $$ = ast;
  }
  | MulExp '/' UnaryExp {
    auto ast = new MulExpAST();
    ast->tag = MulExpAST::DIV;
    ast->mulexp = unique_ptr<MulExpAST>((MulExpAST *)$1);
    ast->unaryexp = unique_ptr<UnaryExpAST>((UnaryExpAST *)$3);
    $$ = ast;
  }
  | MulExp '%' UnaryExp {
    auto ast = new MulExpAST();
    ast->tag = MulExpAST::MOD;
    ast->mulexp = unique_ptr<MulExpAST>((MulExpAST *)$1);
    ast->unaryexp = unique_ptr<UnaryExpAST>((UnaryExpAST *)$3);
    $$ = ast;
  }
  ;

//4.15 OK
RelExp
  : AddExp {
    auto ast = new RelExpAST();
    ast->tag = RelExpAST::EXP;
    ast->addexp = unique_ptr<AddExpAST>((AddExpAST *)$1);
    $$ = ast;
  }
  | RelExp LT AddExp {
    auto ast = new RelExpAST();
    ast->tag = RelExpAST::LT;
    ast->relexp = unique_ptr<RelExpAST>((RelExpAST *)$1);
    ast->addexp = unique_ptr<AddExpAST>((AddExpAST *)$3);
    $$ = ast;
  }
  | RelExp GT AddExp {
    auto ast = new RelExpAST();
    ast->tag = RelExpAST::GT;
    ast->relexp = unique_ptr<RelExpAST>((RelExpAST *)$1);
    ast->addexp = unique_ptr<AddExpAST>((AddExpAST *)$3);
    $$ = ast;
  }
  | RelExp LE AddExp {
    auto ast = new RelExpAST();
    ast->tag = RelExpAST::LE;
    ast->relexp = unique_ptr<RelExpAST>((RelExpAST *)$1);
    ast->addexp = unique_ptr<AddExpAST>((AddExpAST *)$3);
    $$ = ast;
  }
  | RelExp GE AddExp {
    auto ast = new RelExpAST();
    ast->tag = RelExpAST::GE;
    ast->relexp = unique_ptr<RelExpAST>((RelExpAST *)$1);
    ast->addexp = unique_ptr<AddExpAST>((AddExpAST *)$3);
    $$ = ast;
  }
  ;

EqExp
  : RelExp {
    auto ast = new EqExpAST();
    ast->tag = EqExpAST::EXP;
    ast->relexp = unique_ptr<RelExpAST>((RelExpAST *)$1);
    $$ = ast;
  }
  | EqExp EQ RelExp {
    auto ast = new EqExpAST();
    ast->tag = EqExpAST::EQ;
    ast->relexp = unique_ptr<RelExpAST>((RelExpAST *)$3);
    ast->eqexp = unique_ptr<EqExpAST>((EqExpAST *)$1);
    $$ = ast;
  }
  | EqExp NE RelExp {
    auto ast = new EqExpAST();
    ast->tag = EqExpAST::NE;
    ast->relexp = unique_ptr<RelExpAST>((RelExpAST *)$3);
    ast->eqexp = unique_ptr<EqExpAST>((EqExpAST *)$1);
    $$ = ast;
  }
  ;

LAndExp
  : EqExp {
    auto ast = new LAndAST();
    ast->tag = LAndAST::EXP;
    ast->eqexp = unique_ptr<EqExpAST>((EqExpAST *)$1);
    $$ = ast;
  }
  | LAndExp AND EqExp {
    auto ast = new LAndAST();
    ast->tag = LAndAST::AND;
    ast->eqexp = unique_ptr<EqExpAST>((EqExpAST *)$3);
    ast->landexp = unique_ptr<LAndAST>((LAndAST *)$1);
    $$ = ast;
  }
  ;

LOrExp
  : LAndExp {
    auto ast = new LOrAST();
    ast->tag = LOrAST::EXP;
    ast->landexp = unique_ptr<LAndAST>((LAndAST *)$1);
    $$ = ast;
  }
  | LOrExp OR LAndExp {
    auto ast = new LOrAST();
    ast->tag = LOrAST::OR;
    ast->lorexp = unique_ptr<LOrAST>((LOrAST *)$1);
    ast->landexp = unique_ptr<LAndAST>((LAndAST *)$3);
    $$ = ast;
  }
  ;

Number
  : INT_CONST {
    auto ast = new NumberAST();
    ast->val = $1;
    $$ = ast;
  }
  ;

UnaryOp
  : '+'{
    $$ = '+';
  }
  | '-'{
    $$ = '-';
  }
  | '!'{
    $$ = '!';
  }
  ;



%%

void yyerror(std::unique_ptr<BaseAST> &ast, const char *s) {
  cerr << "error: " << s << endl;
}
