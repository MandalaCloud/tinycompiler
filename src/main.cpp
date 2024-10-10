#include <cassert>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <memory>
#include <string>
#include <string.h>
#include <map>
#include <variant>
#include <cstring>
#include "visit.h"
#include "ast.h"

using namespace std;

extern FILE *yyin, *yyout;

int expNumCnt=0, symTabCnt=0, allsymTabCnt=0, ifCnt=0, brctNumCnt=0, whileNumCnt=0;
extern std::string now_while_end="", now_while_entry="", strtmp="", globaltype="";
std::map<std::string, std::pair<int, int>> symbol_table;
std::map<std::string, std::pair<int, int>> *current_table;
std::map<std::string, std::variant<int, std::string>> *curFunvar_table;
std::map<std::map<std::string, std::pair<int, int>>*, std::map<std::string, std::pair<int, int>>*> total_table;
std::map<std::string, std::string> func_table;
std::map<std::string, std::pair<int, int>> glob_table;

std::map<std::string, int> *cur_array_dims_table;
std::map<std::map<std::string, int>*, std::map<std::string, int>*> total_array_dims_table;

extern int yyparse(unique_ptr<BaseAST> &ast);

void generation(const char* str){
  std::cout << "riscv begin!" <<std::endl;
  koopa_program_t program;
  koopa_error_code_t ret = koopa_parse_from_string(str, &program);
  assert(ret == KOOPA_EC_SUCCESS); 
  koopa_raw_program_builder_t builder = koopa_new_raw_program_builder();
  koopa_raw_program_t raw = koopa_build_raw_program(builder, program);
  koopa_delete_program(program);
  Visit(raw);
  std::cout << "riscv visit!" <<std::endl;
  fprintf(yyout, "%s", restr.c_str());
  std::cout << "riscv write!" <<std::endl;
  koopa_delete_raw_program_builder(builder);
}

int main(int argc, const char *argv[]) {
    // 解析命令行参数. 测试脚本/评测平台要求你的编译器能接收如下参数:
    // compiler 模式 输入文件 -o 输出文件
    assert(argc == 5);
    auto mode = argv[1];
    auto input = argv[2];
    auto output = argv[4];

    // 打开输入文件, 并且指定 lexer 在解析的时候读取这个文件
    yyin = fopen(input, "r");
    yyout = fopen(output, "w");
    assert(yyin);
    
    unique_ptr<BaseAST> base_ast;
    unique_ptr<CompUnitAST> ast;
    auto ret = yyparse(base_ast);
    assert(!ret);
    std::cout << "yyparse success!" <<std::endl;

    string str0 = "";
    ast.reset((CompUnitAST *)base_ast.release());
    std::cout << "Reset success!" << std::endl;
    std::cout << "************ Dump Begin! **************" << std::endl;
    ast->Dump(str0);
    std::cout << "Dump success!" << std::endl;
    const char* str1 = str0.c_str();
    std::cout << str0 << std::endl;

    if(strcmp(mode,"-koopa")==0){
      for(int i = 0; i < strlen(str1); i++){
        char c = str1[i];
        if (c =='%') fprintf(yyout,"%%");
        else fprintf(yyout, "%c", c);
      }
    }else generation(str1);

    return 0;
}
