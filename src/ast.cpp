#ifndef AST_CPP
#define AST_CPP

#include "ast.h"

void CompUnitAST::Dump(std::string& str0) const {
  current_table=&symbol_table;

  std::map<std::string, std::variant<int, std::string>> global_var_table;
  curFunvar_table = &global_var_table;

  std::map<std::string, int> global_array_dims_table;
  cur_array_dims_table = &global_array_dims_table;

  str0 += "decl @getint(): i32\n";
  str0 += "decl @getch(): i32\n";
  str0 += "decl @getarray(*i32): i32\n";
  str0 += "decl @putint(i32)\n";
  str0 += "decl @putch(i32)\n";
  str0 += "decl @putarray(i32, *i32)\n";
  str0 += "decl @starttime()\n";
  str0 += "decl @stoptime()\n";

  func_table["getint"] = "int";
  func_table["getch"] = "int";
  func_table["getarray"] = "int";
  func_table["putint"] = "void";
  func_table["putch"] = "void";
  func_table["putarray"] = "void";
  func_table["starttime"] = "void";
  func_table["stoptime"] = "void";

  symTabCnt += 1;
  allsymTabCnt += 1;
  list->Dump(str0);
  symTabCnt -= 1;
}

void CompUnitListAST::Dump(std::string& str0) const{
  for(auto iter = items.begin(); iter != items.end(); iter++)
    (*iter)->Dump(str0);
}

void CompUnitListItemAST::Dump(std::string& str0) const{
  bool flag=true;
  if(tag==DECL) decl->Dump(str0,flag);
  else funcdef->Dump(str0);
}

void FuncDefAST::Dump(std::string& str0) const {
  str0 += "fun @";
  str0 += ident;
  func_table[ident] = type;
  str0 += "( ";
  if(tag==PARAMS) params->Dump(str0);
  str0 += ") ";
  if(type=="int") str0 += ": i32 ";
  str0 += "{\n"; 
  str0 += "\%entry:\n";
  
  std::map<std::string, std::pair<int, int>> new_table;
  std::map<std::string, std::pair<int, int>>* record_table = current_table;
  current_table = &new_table;
  total_table[current_table] = record_table;

  std::map<std::string, std::variant<int, std::string>> new_var_table;
  std::map<std::string, std::variant<int, std::string>> *tmp_var_table = curFunvar_table;
  curFunvar_table = &new_var_table;

  std::map<std::string, int> new_array_dims_table;
  std::map<std::string, int> *tmp_array_dims_table = cur_array_dims_table;
  cur_array_dims_table = &new_array_dims_table;
  total_array_dims_table[cur_array_dims_table] =  tmp_array_dims_table;

  symTabCnt += 1;
  allsymTabCnt += 1;

  if(tag==PARAMS){
    for (auto iter = params->params.begin(); iter != params->params.end(); iter++){
      if((*iter)->tag==FuncFParamAST::IDENT){
        str0 += " @" +  (*iter)->ident + "_" + std::to_string(allsymTabCnt) + " = alloc i32\n";
        str0 += " store @" +  (*iter)->ident + ", @" +  (*iter)->ident + "_" + std::to_string(allsymTabCnt) + '\n';
        std::string new_tab_name = (*iter)->ident+"_"+std::to_string(allsymTabCnt);
        (*current_table)[new_tab_name] = std::make_pair(std::atoi( (*iter)->ident.c_str()), 0);
      }
      else{
        std::string identIndex = (*iter)->ident+'_'+std::to_string(allsymTabCnt);

        int totalDimsNum = 0;
        std::string typeForArray = (*iter)->typeForArrayFParam(totalDimsNum);

        str0 += " @"+identIndex + " = alloc "+typeForArray+"\n";
        str0 += " store @"+(*iter)->ident+", @"+identIndex+'\n';
        (*current_table)[identIndex] = std::make_pair(std::atoi( (*iter)->ident.c_str()), 3);//ArrayParam
        (*cur_array_dims_table)[identIndex] = totalDimsNum;
      }
    }
  }

  block->type=type;
  block->Dump(str0);
  std::string str ;
  str = str0.substr(str0.length()-3, str0.length());
  if(type=="void") str0 += " ret \n";
  else if(str == ":\n\n") str0 += " ret 0\n";
  str0 += "}\n";

  current_table = record_table;
  curFunvar_table = tmp_var_table;
  cur_array_dims_table = tmp_array_dims_table;
  symTabCnt -= 1;
}

std::vector<int> FuncFParamAST::const_value(){
  std::vector<int> tmp;
  for(auto iter = constexps.begin(); iter != constexps.end(); iter++){
    (*iter)->exp->DumpStr(strtmp);
    tmp.emplace_back((*iter)->exp->val);
  }
  return tmp;
}

std::string FuncFParamAST::typeForArrayFParam(int& totalDimsNum){
  std::string resStr;
  std::vector<int> dims = const_value();
  int numOfArrayVector = dims.size();
  totalDimsNum += numOfArrayVector + 1;
  if(numOfArrayVector == 0){
    resStr += "*i32";
  }else if(numOfArrayVector > 0){
    std::string tmpvecstr = "[i32, "+ std::to_string(dims[numOfArrayVector - 1]) + "]";
    for(int i = numOfArrayVector - 2; i >= 0; i--)
    {
      tmpvecstr = "[" + tmpvecstr +", "+std::to_string(dims[i])+"]";
    }
    resStr += "*";
    resStr += tmpvecstr;
  }
  return resStr;
}

void FuncFParamAST::Dump(std::string& str0){
  str0 += "@" + ident + ": ";
  if(tag==IDENT) str0 += "i32";
  else{
    int tmp = 0;
    str0 += typeForArrayFParam(tmp);
  }
}

void FuncFParamsAST::Dump(std::string& str0) const{
  if(params.empty()) return ;
  for(int i = 0; i < params.size(); i++){
    params[i]-> Dump(str0);
    if(i!=params.size()-1) str0 += ", ";
  }
}

void FuncTypeAST::Dump(std::string& str0) const {
  if(type=="int") str0 += ": i32 ";
}

void BlockAST::Dump(std::string& str0) {
  isret = false ;

  // 每一块需要自己的符号表
  std::map<std::string, std::pair<int, int>> new_table;
  std::map<std::string, std::pair<int, int>>* record_table = current_table;
  current_table = &new_table;
  total_table[current_table] = record_table;

  std::map<std::string, int> new_array_dims_table;
  std::map<std::string, int> *tmp_array_dims_table = cur_array_dims_table;
  cur_array_dims_table = &new_array_dims_table;
  total_array_dims_table[cur_array_dims_table] =  tmp_array_dims_table;

  symTabCnt += 1;
  allsymTabCnt += 1;

  for(int i = 0; i < blockitems.size(); i++){
    if(blockitems[i]->tag==BlockItemAST::STMT) blockitems[i]->stmt->type=type;
    if(isret) return ;
    blockitems[i]->Dump(str0);
    if(blockitems[i]->tag==BlockItemAST::STMT)
      if(blockitems[i]->stmt->isret){
        isret = true ;
        break;
      }
  }

  str0+="\n";
  current_table = record_table;
  cur_array_dims_table = tmp_array_dims_table;
  symTabCnt -= 1;
  allsymTabCnt += 1;
  // std::cout << str0 << std::endl;
}

void BlockItemAST::Dump(std::string& str0) const{
  if(tag==DECL) decl->Dump(str0);
  else stmt->Dump(str0);
  // std::cout << str0 << std::endl;
}

void DeclAST::Dump(std::string& str0,bool isglobal) const{
  if(tag==CONST) constdecl->Dump(str0,isglobal);
  else vardecl->Dump(str0,isglobal);
  // std::cout << str0 << std::endl;
}

void ConstDeclAST::Dump(std::string& str0,bool isglobal) const{
  for(auto iter = constdefs.begin(); iter != constdefs.end(); iter++)
    (*iter)->Dump(str0,isglobal);
  // std::cout << str0 << std::endl;
}

std::vector<int> ConstDefAST::const_value(){
  std::vector<int> tmp;
  for(auto iter = constexps.begin(); iter != constexps.end(); iter++){
    (*iter)->exp->DumpStr(strtmp);
    tmp.emplace_back((*iter)->exp->val);
  }
  return tmp;
}

void ConstDefAST::Dump(std::string& str0,bool ifglobal=true){
  std::string identtmp = ident + '_' + std::to_string(allsymTabCnt);
  if(tag==EQ){
    std::string tmp = constinitval->constexp->exp->DumpStr(strtmp);
    (*current_table)[identtmp] = std::make_pair(constinitval->constexp->exp->val, 1);
  }else{
    std::vector<int> dims = const_value();
    if((*curFunvar_table).find(identtmp) ==  (*curFunvar_table).end()){
      if(ifglobal) str0 += "global" ;
      str0 += " @" + identtmp + " = alloc ";
      int numOfArrayVector = constexps.size();
      std::string tmpvecstr = "[i32, "+ std::to_string(constexps[numOfArrayVector-1]->exp->val) + "]";
      for(int i = numOfArrayVector - 2; i >= 0; i--){
        tmpvecstr = "[" + tmpvecstr +", "+std::to_string(constexps[i]->exp->val)+"]";
      }
      str0 += tmpvecstr ;
      (*curFunvar_table)[identtmp] = "used";
    }
   
    std::vector<int> numForEachDim;
    numForEachDim.push_back(1);
    int mulRes = 1;
    for(int i = constexps.size() - 1; i > 0; i--){
      mulRes *= constexps[i]->exp->val;
      numForEachDim.push_back(mulRes);
    }
    mulRes *= constexps[0]->exp->val;
    reverse(numForEachDim.begin(), numForEachDim.end());

    if(constinitval->tag!=ConstInitValAST::EMPTY){
      std::vector<std::string> initValArray;
      constinitval->getInitValArray(initValArray, numForEachDim,0,0,str0);
      while(initValArray.size() < mulRes){
        initValArray.push_back("0");
      }

      if(ifglobal){ 
        str0 += ", ";
        globalInitSearch(dims, numForEachDim, 0, str0, initValArray, 0);
      }else{
        str0 += "\n";
        getElemPtr(identtmp, dims, numForEachDim, 0, str0, initValArray, 0);
      }
      str0 += "\n";
    }else{
      if(ifglobal){
        str0 += ", zeroinit\n";
      }else{
        str0 += "\n";
        std::vector<std::string> initValArray;
        for(int i = 0; i < mulRes; i++){
          initValArray.push_back("0");
        }
        getElemPtr(identtmp, dims, numForEachDim, 0, str0, initValArray, 0);
      }     
    }
    (*current_table)[identtmp] = std::make_pair(0, 2);
    (*cur_array_dims_table)[identtmp] = constexps.size();
    (*curFunvar_table)[identtmp] = "used";
  }
  // std::cout << str0 << std::endl;
}

void ConstDefAST::getElemPtr(std::string ptrIdent, std::vector<int>& dims, std::vector<int>& numsForDims, int curPtrIndex, std::string& str0, std::vector<std::string>& initArray, int initValIndex){
  for(int i = 0; i < dims[curPtrIndex]; i++){
    std::string regForPtr = '%'+std::to_string(allsymTabCnt);
    allsymTabCnt++;
    str0 += " "+regForPtr+" = getelemptr ";
    if(curPtrIndex==0) str0 += "@";
    str0 += ptrIdent+", "+std::to_string(i)+"\n";
    int tmpInitValIndex = initValIndex + numsForDims[curPtrIndex]*i;
    if(curPtrIndex+1 == dims.size()){
      str0 += " store "+initArray[tmpInitValIndex]+", "+regForPtr+"\n";
    }else{
      getElemPtr(regForPtr, dims, numsForDims, curPtrIndex+1, str0, initArray, tmpInitValIndex);
    }
  }
}

void ConstDefAST::globalInitSearch(std::vector<int>& dims, std::vector<int>& numsForDims, int curDimIndex, std::string& str0, std::vector<std::string>& initArray, int initValIndex){
  str0 += "{";
  for(int i = 0; i < dims[curDimIndex]; i++){
    if(i != 0){
      str0 += ",";
    }
    int tmpInitValIndex = initValIndex + numsForDims[curDimIndex]*i;
    if(curDimIndex + 1 == dims.size()){
      str0 += initArray[tmpInitValIndex];
    }else{
      globalInitSearch(dims, numsForDims, curDimIndex+1, str0, initArray, tmpInitValIndex);
    }
  }
  str0 += "}";
}

void ConstInitValAST::getInitValArray(std::vector<std::string>& resVec, std::vector<int>& numsForDim, int brace, int align, std::string& str0){
  if(tag==EXP){
    std::string tmp = constexp->exp->DumpStr(strtmp);
    resVec.push_back(tmp);
  }
  else if(tag==EMPTY){
    brace++;
    int i = align+brace;
    int filled = resVec.size();
    if(filled == 0) i = brace-1;
    int stuff = numsForDim[i];
    while(stuff>0){
      resVec.push_back("0");stuff--;
    }
  }else{
    brace++;
    int filled = resVec.size();
    int i = 0;
    if(filled == 0) i=brace-1;
    else while(filled%numsForDim[i] != 0) i++;
    int before = resVec.size();
    for(int j=0; j<inits.size(); j++){
      inits[j]->getInitValArray(resVec, numsForDim, brace, i, str0);
      brace = 0;
    }
    int after = resVec.size();
    while(after<before+numsForDim[i]){
      after++;
      resVec.push_back("0");
    }
  }
  // for(int i=0; i<resVec.size(); i++){
  //   std::cout << resVec[i] << " " << "!" << std::endl;
  // }
}

void VarDeclAST::Dump(std::string& str0,bool isglobal) const{
  for(auto iter = vardefs.begin(); iter != vardefs.end(); iter++)
    (*iter)->Dump(str0,isglobal);
  // std::cout << str0 << std::endl;
}

std::vector<int> VarDefAST::const_value(){
  std::vector<int> tmp;
  for(auto iter = constexps.begin(); iter != constexps.end(); iter++){
    (*iter)->exp->DumpStr(strtmp);
    tmp.emplace_back((*iter)->exp->val);
  }
  return tmp;
}

void VarDefAST::Dump(std::string& str0,bool ifglobal){
  std::string identtmp = ident + "_" + std::to_string(allsymTabCnt);
  if(tag==EQ||tag==IDENT){
    if(symTabCnt!=1){
      str0 += " @";
      str0 += identtmp;
      str0 += " = alloc i32\n";

      if (tag==EQ){
        std::string tmp = initval->DumpStr(str0);
        str0 += " store ";
        str0 += tmp;
        str0 += ", @";
        str0 += identtmp;
        str0 += "\n";

        (*current_table)[identtmp] = std::make_pair(initval->exp->val, 0);
      }else
        (*current_table)[identtmp] = std::make_pair(0, 0);
    }else{
      str0 += "global @";
      str0 += identtmp;
      str0 += " = alloc i32, ";

      if (tag==EQ){
        std::string globalVarVal = initval->DumpStr(str0);
        str0 += globalVarVal;
        str0 += "\n";
        (*current_table)[identtmp] = std::make_pair(initval->exp->val, 0);
        glob_table[identtmp] = std::make_pair(initval->exp->val, 0);
      }
      else{
          str0 += "zeroinit\n";
          (*current_table)[identtmp] = std::make_pair(0, 0);
          glob_table[identtmp] = std::make_pair(0, 0);
      }
    }
    (*curFunvar_table)[identtmp] = "used";
  }else{
    std::vector<int>dims = const_value();
    if((*curFunvar_table).find(identtmp) ==  (*curFunvar_table).end()){
      if(ifglobal) str0 += "global";
      str0 += " @" + identtmp + " = alloc ";
      int numOfArrayVector = constexps.size();
      std::string tmpvecstr = "[i32, "+ std::to_string(constexps[numOfArrayVector-1]->exp->val) + "]";
      for(int i = numOfArrayVector - 2; i >= 0; i--){
        tmpvecstr = "[" + tmpvecstr +", "+std::to_string(constexps[i]->exp->val)+"]";
      }
      str0 += tmpvecstr;
      (*curFunvar_table)[identtmp] = "used";
    }
   
    std::vector<int> numForEachDim;
    numForEachDim.push_back(1);
    int mulRes = 1;
    for(int i = constexps.size() - 1; i > 0; i--){
      mulRes *= constexps[i]->exp->val;
      numForEachDim.push_back(mulRes);
    }
    mulRes *= constexps[0]->exp->val;
    reverse(numForEachDim.begin(), numForEachDim.end());

    if(tag==EQARRAY&&initval->tag!=InitValAST::EMPTY){
      std::vector<std::string> initValArray;
      initval->getInitValArray(initValArray, numForEachDim, 0, 0,str0);
      while(initValArray.size() < mulRes){
        initValArray.push_back("0");
      }

      if(ifglobal){
        str0 += ", ";
        globalInitSearch(dims, numForEachDim, 0, str0, initValArray, 0);
      }
      else{
        str0 += '\n';
        getElemPtr(identtmp, dims, numForEachDim, 0, str0, initValArray, 0);
      }
      str0 += "\n";
    }else{
      if(ifglobal){
        str0 += ", zeroinit\n";
      }else{
        str0 += "\n";
        std::vector<std::string> initValArray;
        for(int i = 0; i < mulRes; i++){
          initValArray.push_back("0");
        }
        getElemPtr(identtmp, dims, numForEachDim, 0, str0, initValArray, 0);
      }
    }
    (*current_table)[identtmp] = std::make_pair(0, 2);
    (*cur_array_dims_table)[identtmp] = constexps.size();
    (*curFunvar_table)[identtmp] = "used";
  }
  // std::cout << str0 << std::endl;
}

void VarDefAST::getElemPtr(std::string ptrIdent, std::vector<int>& dims, std::vector<int>& numsForDims, int curPtrIndex, std::string& str0, std::vector<std::string>& initArray, int initValIndex){
  for(int i = 0; i < dims[curPtrIndex]; i++){
    std::string regForPtr = '%'+std::to_string(allsymTabCnt);
    allsymTabCnt++;
    str0 += " "+regForPtr+" = getelemptr ";
    if(curPtrIndex==0) str0 += "@";
    str0 += ptrIdent+", "+std::to_string(i)+"\n";
    int tmpInitValIndex = initValIndex + numsForDims[curPtrIndex]*i;
    if(curPtrIndex+1 == dims.size()){
      str0 += " store "+initArray[tmpInitValIndex]+", "+regForPtr+"\n";
    }else{
      getElemPtr(regForPtr, dims, numsForDims, curPtrIndex+1, str0, initArray, tmpInitValIndex);
    }
  }
}

void VarDefAST::globalInitSearch(std::vector<int>& dims, std::vector<int>& numsForDims, int curDimIndex, std::string& str0, std::vector<std::string>& initArray, int initValIndex){
  str0 += "{";
  for(int i = 0; i < dims[curDimIndex]; i++){
    if(i != 0){
      str0 += ",";
    }
    int tmpInitValIndex = initValIndex + numsForDims[curDimIndex]*i;
    if(curDimIndex + 1 == dims.size()){
      str0 += initArray[tmpInitValIndex];
    }else{
      globalInitSearch(dims, numsForDims, curDimIndex+1, str0, initArray, tmpInitValIndex);
    }
  }
  str0 += "}";
}

std::string InitValAST::DumpStr(std::string& str0){
  return exp->DumpStr(str0);
}

void InitValAST::getInitValArray(std::vector<std::string>& resVec, std::vector<int>& numsForDim, int brace, int align, std::string& str0){
  if(tag==EXP){
    std::string tmp = exp->DumpStr(str0);
    resVec.push_back(tmp);
  }
  else if(tag==EMPTY){
    brace++;
    int i = align+brace;
    int filled = resVec.size();
    if(filled == 0) i = brace-1;
    int stuff = numsForDim[i];
    while(stuff>0){
      resVec.push_back("0");stuff--;
    }
  }else{
    brace++;
    int filled = resVec.size();
    int i = 0;
    if(filled == 0) i=brace-1;
    else while(filled%numsForDim[i] != 0) i++;
    int before = resVec.size();
    for(int j=0; j<inits.size(); j++){
      inits[j]->getInitValArray(resVec, numsForDim, brace, i, str0);
      brace = 0;
    }
    int after = resVec.size();
    while(after<before+numsForDim[i]){
      after++;
      resVec.push_back("0");
    }
  }
  // for(int i=0; i<resVec.size(); i++){
  //   std::cout << resVec[i] << " " << "!" << std::endl;
  // }
}

std::string ConstInitValAST::DumpStr(std::string& str0){
  return constexp->DumpStr(str0);
}

std::string ConstExpAST::DumpStr(std::string& str0){
  return exp->DumpStr(str0);
}

void StmtAST::Dump(std::string& str0) {
  if( tag== RET_EXP ){
    std::string tmp = exp->DumpStr(str0);
    str0 += " ret ";
    str0 += tmp;
    str0 += "\n";
  }else if (tag == RET ){
    str0 += " ret";
    if(type=="int") str0+= " 0";
    str0 += "\n";
  }else if (tag == BLOCK){
    block->type=type;
    block->Dump(str0);
    isret = block->isret;
  }else if (tag == EXP){
    std::string tmp = exp->DumpStr(str0);
  }else if (tag == EQ){
    if(lval->tag==LValAST::IDENT){
      std::string exptmp = exp->DumpStr(str0);
      std::string lvaltmp = lval->DumpStr(str0);
      std::string tmp_num;

      int searchdep = symTabCnt;
      std::map<std::string, std::pair<int, int>> *search_table = current_table;
      while (searchdep > 0){   
        int flag = 0;
        for (int ident_idx = allsymTabCnt; ident_idx >= searchdep; ident_idx--){
          tmp_num = lvaltmp + '_' + std::to_string(ident_idx);
          if ((*search_table).find(tmp_num) != (*search_table).end()) flag = 1;
          if (flag) break;
        }
        
        if (flag) break;
        search_table = total_table[search_table];
        searchdep -= 1;
      }

      str0 += " store ";
      str0 += exptmp;
      str0 += ", @";
      str0 += tmp_num;
      str0 += "\n";
      // std::cout << "****** renew symbol table: " << lvaltmp << " → " << std::to_string(exp->val) << std::endl;

      lval->val = exp->val;
    }else{
      bool isArrayFunParam = false;
      std::string resident;
      std::string exptmp = exp->DumpStr(str0);
      std::string lvaltmp = lval->DumpStr(str0);
      std::string tmp_num;
      int searchdep = symTabCnt;
      std::map<std::string, std::pair<int, int>> *search_table = current_table;
      std::map<std::string, int> *search_array_dims_table = cur_array_dims_table;
      
      bool flag = false;
      while(searchdep > 0){
        for (int ident_idx = allsymTabCnt; ident_idx >= searchdep; ident_idx--){
          std::string tmptmpident = lvaltmp + '_' + std::to_string(ident_idx);
          if((*search_table).find(tmptmpident) != (*search_table).end()){
            resident = tmptmpident;
            flag=true;
            if((*search_table)[resident].second == 3) isArrayFunParam = true;
            break;
          }
        }
        if(flag) break;
        search_table = total_table[search_table];
        search_array_dims_table = total_array_dims_table[search_array_dims_table];
        searchdep -= 1;
      }
      if(searchdep == 0){
        std::cout<<"WRONG AT FIND IDENT"<<std::endl;
      }
      std::string ptrForLVal = lval->getElemPtrForLVal(str0, resident, isArrayFunParam);
      str0 += " store " + exptmp + ", " + ptrForLVal+'\n';
      str0 += '\n';
    }
  }else if(tag == IF){
    stmt1->type=type;
    std::string exptmp = exp->DumpStr(str0);
    int tmpifCnt = ifCnt;
    ifCnt++;

    str0 += " br ";
    str0 += exptmp;
    str0 += ", \%then_";
    str0 += std::to_string(tmpifCnt);
    str0 += ", ";
    str0 += "\%end_";
    str0 += std::to_string(tmpifCnt);
    str0 += "\n\n";

    str0 += "\%then_";
    str0 += std::to_string(tmpifCnt);
    str0 += ":\n";
    stmt1->Dump(str0);

    if (!stmt1->isret){
      str0 += " jump \%end_";
      str0 += std::to_string(tmpifCnt);
      str0 += "\n\n";
    }

    str0 += "\%end_";
    str0 += std::to_string(tmpifCnt);
    str0 += ":\n";

  }else if(tag == IFELSE){
    stmt1->type=type;
    stmt2->type=type;
    std::string exptmp = exp->DumpStr(str0);
    int tmpifCnt = ifCnt;
    ifCnt++;

    str0 += " br ";
    str0 += exptmp;
    str0 += ", \%then_";
    str0 += std::to_string(tmpifCnt);
    str0 += ", ";
    str0 += "\%else_";
    str0 += std::to_string(tmpifCnt);
    str0 += "\n\n";

    str0 += "\%then_";
    str0 += std::to_string(tmpifCnt);
    str0 += ":\n";
    stmt1->Dump(str0);

    if (!stmt1->isret){
      str0 += " jump \%end_";
      str0 += std::to_string(tmpifCnt);
      str0 += "\n\n";
    }

    str0 += "\%else_";
    str0 += std::to_string(tmpifCnt);
    str0 += ":\n";
    stmt2->Dump(str0);

    if (!stmt2->isret){
      str0 += " jump \%end_";
      str0 += std::to_string(tmpifCnt);
      str0 += "\n\n";
    }

    str0 += "\%end_";
    str0 += std::to_string(tmpifCnt);
    str0 += ":\n";
  }else if( tag == WHILE ){
    stmt1->type=type;
    int whileNumCnt_tmp = whileNumCnt;
    whileNumCnt += 1;
    std::string now_while_end_tmp = now_while_end;
    std::string now_while_entry_tmp = now_while_entry;

    now_while_end = "\%while_end_" + std::to_string(whileNumCnt_tmp);
    now_while_entry = "\%while_entry_" + std::to_string(whileNumCnt_tmp);

    str0 += " jump \%while_entry_";
    str0 += std::to_string(whileNumCnt_tmp);
    str0 += "\n";

    str0 += "\%while_entry_";
    str0 += std::to_string(whileNumCnt_tmp);
    str0 += ":\n";

    std::string tmp1 = exp->DumpStr(str0);
    str0 += " br ";
    str0 += tmp1;
    str0 += ", \%while_body_";
    str0 += std::to_string(whileNumCnt_tmp);
    str0 += ", \%while_end_";
    str0 += std::to_string(whileNumCnt_tmp);
    str0 += "\n";

    str0 += "\%while_body_";
    str0 += std::to_string(whileNumCnt_tmp);
    str0 += ":\n";
    stmt1->Dump(str0);

    if (!stmt1->isret){
        str0 += " jump \%while_entry_";
        str0 += std::to_string(whileNumCnt_tmp);
        str0 += "\n";
    }

    str0 += "\%while_end_";
    str0 += std::to_string(whileNumCnt_tmp);
    str0 += ":\n";

    now_while_end = now_while_end_tmp;
    now_while_entry = now_while_entry_tmp;

  }else if( tag == BREAK ){
    str0 += " jump ";
    str0 += now_while_end;
    str0 += "\n";

    str0 += "\%while_body1_";
    str0 += std::to_string(brctNumCnt);
    str0 += ":\n";
    brctNumCnt += 1;
  }else if( tag == CONTINUE ){
    str0 += " jump ";
    str0 += now_while_entry;
    str0 += "\n";

    str0 += "\%while_body1_";
    str0 += std::to_string(brctNumCnt);
    str0 += ":\n";
    brctNumCnt += 1;
  }
  // std::cout << str0 << std::endl;
}

std::string ExpAST::DumpStr(std::string& str0) {
  // std::cout << str0 << std::endl;
  std::string tmp = lorexp->DumpStr(str0);
  val = lorexp->val;
  return tmp;
}

std::string PrimaryExpAST::DumpStr(std::string& str0) {
  // std::cout << "PrimaryExpAST" << std::endl;
  // std::cout << str0 << std::endl;
  if(tag==EXP){
    std::string tmp = exp->DumpStr(str0);
    val = exp->val;
    // std::cout << val << std::endl;
    return tmp;
  }
  else if(tag==NUM){
    val=number->val;
    // std::cout << val << std::endl;
    return number->DumpStr(str0);
  }
  else{
    if(lval->tag==LValAST::IDENT){
      std::string tmp , tmp1 ;
      tmp = lval->DumpStr(str0);
      std::string identtmp = tmp , identtmp_num;
      int searchdep = symTabCnt , fullArrayDimsNum=0;
      std::map<std::string, std::pair<int, int>> *search_table = current_table;
      std::map<std::string, int> *search_array_dims_table = cur_array_dims_table;

      bool isArrayFunParam = false , isArray = false;
      while (searchdep > 0){   
        int flag = 0;
        for (int ident_idx = allsymTabCnt; ident_idx >= searchdep; ident_idx--){
          identtmp_num = identtmp + '_' + std::to_string(ident_idx);
          if ((*search_table).find(identtmp_num) != (*search_table).end()){
            flag = 1;
            int mood = (*search_table)[identtmp_num].second;
            if(mood==3) isArrayFunParam = true;
            if(mood==3||mood==2) isArray = true;
            break;
          }
        }
        if (flag) break;
        search_table = total_table[search_table];
        search_array_dims_table = total_array_dims_table[search_array_dims_table];
        searchdep -= 1;
      }
      if(isArray){
        fullArrayDimsNum = (*search_array_dims_table)[identtmp_num];
        int curArrayDims = 0;

        std::string ptrIdent = lval->getElemPtrForLVal(str0, identtmp_num, isArrayFunParam);

        if(isArrayFunParam == false){
          std::string ptrnum = "0";
          std::string regForPtr = '%'+std::to_string(allsymTabCnt);
          allsymTabCnt++;
          str0 += " "+regForPtr+" = getelemptr "+ptrIdent+", "+ptrnum+"\n";

          ptrIdent = regForPtr;
        }else if(isArrayFunParam == true){
          std::string regToLoadArrayParam = '%'+std::to_string(allsymTabCnt);
          allsymTabCnt++;
          str0 += " "+regToLoadArrayParam+" = load @"+identtmp_num+"\n";
          std::string regToGetPtrArrayParam = '%'+std::to_string(allsymTabCnt);
          allsymTabCnt++;
          std::string ptrnum0 = "0";
          str0 += " "+regToGetPtrArrayParam+" = getptr "+regToLoadArrayParam+", "+ptrnum0+"\n";
      
          ptrIdent = regToGetPtrArrayParam;
        }
        return ptrIdent;
      }else{        
        auto sym_label_val = (*search_table)[identtmp_num];
        val=sym_label_val.first;
        if (searchdep == 0){
            auto sym_label_val = symbol_table[tmp1];
            return std::to_string(sym_label_val.first);
        }
        if(sym_label_val.second==0){
          tmp1 = "%" + std::to_string(allsymTabCnt++);
          str0 += " ";
          str0 += tmp1;
          str0 += " = load @";
          str0 += identtmp_num;
          str0 += "\n";
        }
        else tmp1 = std::to_string(sym_label_val.first);
        // std::cout << val << std::endl;
        return tmp1;
      }
    }else{
      bool isArrayFunParam = false;
      std::string tmpident = lval->ident;
      std::string resident;
      int searchdep = symTabCnt;
      int fullArrayDimsNum = 0;
      std::map<std::string, std::pair<int, int>> *search_table = current_table;
      std::map<std::string, int> *search_array_dims_table = cur_array_dims_table;

      // std::cout<<"start find tmpident"<<std::endl;
      bool flag=false;
      while(searchdep>0){
        for (int ident_idx = allsymTabCnt; ident_idx >= searchdep; ident_idx--){
          std::string tmptmpident = tmpident + '_' + std::to_string(ident_idx); 
          if((*search_table).find(tmptmpident) != (*search_table).end()){
            flag=true;
            resident = tmptmpident;
            // std::cout<<resident<<std::endl;
            if((*search_table)[resident].second == 3) isArrayFunParam = true;
            fullArrayDimsNum = (*search_array_dims_table)[resident];
            // std::cout<<fullArrayDimsNum<<std::endl;
            break;
          }           
        }
        if(flag) break;
        search_table = total_table[search_table];
        search_array_dims_table = total_array_dims_table[search_array_dims_table];
        searchdep -= 1; 
      }

      // std::cout<<"!!!"<<std::endl;
      if(searchdep == 0){
        std::cout<<"WRONG AT FIND IDENT"<<std::endl;
      }

      int curLValDimsNum = lval->exps.size();
      // std::cout<<fullArrayDimsNum<<" "<<curLValDimsNum<<std::endl;

      std::string ptrIdent = lval->getElemPtrForLVal(str0, resident, isArrayFunParam);

      if(curLValDimsNum == fullArrayDimsNum){
        std::string valueForLValArray = '%'+std::to_string(allsymTabCnt);
        allsymTabCnt++;
        str0 += " "+valueForLValArray+" = load "+ptrIdent+"\n";
        return valueForLValArray;
      }else if(curLValDimsNum < fullArrayDimsNum){
        std::string ptrnum = "0";
        std::string regForPtr = '%'+std::to_string(allsymTabCnt);
        allsymTabCnt++;
        str0 += " "+regForPtr+" = getelemptr "+ptrIdent+", "+ptrnum+"\n";
        ptrIdent = regForPtr;
        return ptrIdent;
      }else{
        // assert(false);
        std::cout << "lval primary error" << std::endl;
      }
    }
  }
}

std::string LValAST::DumpStr(std::string& str0) {
  return ident;
}

std::string LValAST::getElemPtrForLVal(std::string& str0, std::string& resident, bool isArrayFunParam) {
  std::string ptrIdent = '@'+resident;
  if(isArrayFunParam == false){
    if(tag==ARRAY){
      for(int i = 0; i < exps.size(); i++){
        std::string ptrnum = exps[i]->DumpStr(str0);
        std::string regForPtr = '%'+std::to_string(allsymTabCnt);
        allsymTabCnt++;
        str0 += " "+regForPtr+" = getelemptr "+ptrIdent+", "+ptrnum+"\n";
        ptrIdent = regForPtr;
      }
    }
  }else if(isArrayFunParam == true){
    if(tag==ARRAY){
      std::string regToLoadArrayParam = '%'+std::to_string(allsymTabCnt);
      allsymTabCnt++;
      str0 += " "+regToLoadArrayParam+" = load @"+resident+"\n";
      std::string regToGetPtrArrayParam = '%'+std::to_string(allsymTabCnt);
      allsymTabCnt++;
      std::string ptrnum0 = exps[0]->DumpStr(str0);
      str0 += " "+regToGetPtrArrayParam+" = getptr "+regToLoadArrayParam+", "+ptrnum0+"\n";
  
      ptrIdent = regToGetPtrArrayParam;
      for(int i = 1; i < exps.size(); i++){
        std::string ptrnum = exps[i]->DumpStr(str0);
        std::string regForPtr = '%'+std::to_string(allsymTabCnt);
        allsymTabCnt++;
        str0 += " "+regForPtr+" = getelemptr "+ptrIdent+", "+ptrnum+"\n";
        ptrIdent = regForPtr;
      }
    }
  }
  return ptrIdent;
  // std::cout << "LVal Error" << std::endl;
}

std::string UnaryExpAST::DumpStr(std::string& str0) {
  // std::cout << "UnaryExpAST" << std::endl;
  if(tag==EXP){
    std::string tmp = primaryexp->DumpStr(str0);
    val = primaryexp->val;
    // std::cout << val << std::endl;
    return tmp;
  }
  else if (tag == OP){
    std::string tmp, str1;
    if(unaryop=='+'){
      tmp = unaryexp->DumpStr(str0);
      val=unaryexp->val;
    }
    else{
      str1 = unaryexp->DumpStr(str0);
      tmp = "%" + std::to_string(allsymTabCnt++);

      str0 += " ";
      str0 += tmp;
      str0 += " = ";

      if(unaryop=='-'){
        str0 += "sub ";
        val=0-unaryexp->val;
      }
      else{
        str0 += "eq ";
        val=(unaryexp->val==0);
      }

      str0 += "0, ";
      str0 += str1;
      str0 += "\n";
    }
    // std::cout << str0 << std::endl;
    // std::cout << val << std::endl;
    return tmp;
  }else{
    std::string funcret;
    if (func_table[ident] == "int"){
      funcret = "\%" + std::to_string(allsymTabCnt++);
      if (tag==PARAMS){
        std::vector<std::string> allFuncParams;
        for (auto iter = params->exps.begin(); iter != params->exps.end(); iter++){
            std::string funcParam = (*iter)->DumpStr(str0);
            allFuncParams.push_back(funcParam);
        }

        str0 += " ";
        str0 += funcret;
        str0 += " = call @";
        str0 += ident;
        str0 += "(";

        for (int i = 0; i < allFuncParams.size(); i++){
          if (i) str0 += ", ";
          str0 += allFuncParams[i];
        }
        str0 += ")\n";
      }else{
          str0 += " ";
          str0 += funcret;
          str0 += " = call @";
          str0 += ident;
          str0 += "()\n";
      }
    }else if (func_table[ident] == "void"){
      funcret = "";
      if (tag==PARAMS){
        std::vector<std::string> allFuncParams;
        for (auto iter = params->exps.begin(); iter != params->exps.end(); iter++){
          std::string funcParam = (*iter)->DumpStr(str0);
          allFuncParams.push_back(funcParam);
        }

        str0 += " call @";
        str0 += ident;
        str0 += "(";

        for (int i = 0; i < allFuncParams.size(); i++){
            if (i) str0 += ", ";
            str0 += allFuncParams[i];
        }
        str0 += ")\n";
      }else{
          str0 += " call @";
          str0 += ident;
          str0 += "()\n";
      }
    }
    return funcret;
  }
}

void FuncRParamsAST:: Dump(std::string& str0) const{
  for (auto iter = exps.begin(); iter != exps.end(); iter++){
    std::string tmp = (*iter)->DumpStr(str0);
    if (iter != exps.begin()) str0 += ", ";
    str0 += tmp;
  }
}

std::string MulExpAST::DumpStr(std::string& str0) {
  // std::cout << "MulExpAST" << std::endl;
  if(tag==EXP){
    std::string tmp=unaryexp->DumpStr(str0);
    val=unaryexp->val;
    // std::cout << val << std::endl;
    return tmp;
  }
  else{
    std::string tmp , str1 , str2 ;
    str1 = mulexp->DumpStr(str0);
    str2 = unaryexp->DumpStr(str0);
    tmp = "%" + std::to_string(allsymTabCnt++);

    str0 += " ";
    str0 += tmp;
    str0 += " = ";   

    switch(tag){
      case MUL : str0 += "mul "; val=mulexp->val*unaryexp->val; break;
      case DIV : str0 += "div "; if(unaryexp->val) val=mulexp->val/unaryexp->val; break;
      case MOD : str0 += "mod "; if(unaryexp->val) val=mulexp->val%unaryexp->val; break;
      default : break;
    }  

    str0 += str1;
    str0 += ", ";
    str0 += str2;
    str0 += "\n";

    // std::cout << str0 << std::endl;
    // std::cout << val << std::endl;
    return tmp;
  }
}

std::string AddExpAST::DumpStr(std::string& str0) {
  // std::cout << "AddExpAST" << std::endl;
  if(tag==EXP){
    std::string tmp = mulexp->DumpStr(str0);
    val=mulexp->val;
    std::cout << val << std::endl;
    return tmp;
  }
  else{
    std::string tmp , str1 , str2 ;
    str1 = addexp1->DumpStr(str0);
    str2 = mulexp1->DumpStr(str0);
    tmp = "%" + std::to_string(allsymTabCnt++);

    str0 += " ";
    str0 += tmp;
    str0 += " = ";   

    switch(tag){
      case ADD : str0 += "add "; val=addexp1->val+mulexp1->val; break;
      case SUB : str0 += "sub "; val=addexp1->val-mulexp1->val; break;
      default : break;
    }  

    str0 += str1;
    str0 += ", ";
    str0 += str2;
    str0 += "\n";

    // std::cout << str0 << std::endl;
    // std::cout << val << std::endl;
    return tmp;
  }
}

std::string RelExpAST::DumpStr(std::string& str0) {
  // std::cout << "RelExpAST" << std::endl;
  if(tag==EXP){
    std::string tmp = addexp->DumpStr(str0);
    val = addexp->val;
    std::cout << val << std::endl;
    return tmp;
  }
  else{
    std::string tmp , str1 , str2 ;
    str1 = relexp->DumpStr(str0);
    str2 = addexp->DumpStr(str0);
    tmp = "%" + std::to_string(allsymTabCnt++);

    str0 += " ";
    str0 += tmp;
    str0 += " = ";   

    switch(tag){
      case LT : str0 += "lt "; val=(relexp->val<addexp->val); break;
      case LE : str0 += "le "; val=(relexp->val<=addexp->val); break;
      case GT : str0 += "gt "; val=(relexp->val>addexp->val); break;
      case GE : str0 += "ge "; val=(relexp->val>=addexp->val); break;
      default : break;
    }  

    str0 += " ";
    str0 += str1;
    str0 += ", ";
    str0 += str2;
    str0 += "\n";

    // std::cout << str0 << std::endl;
    // std::cout << val << std::endl;
    return tmp;
  }
}

std::string EqExpAST::DumpStr(std::string& str0) {
  // std::cout << "EqExpAST" << std::endl;
  if(tag==EXP){
    std::string tmp = relexp->DumpStr(str0);
    val = relexp->val;
    std::cout << val << std::endl;
    return tmp;
  }
  else{
    std::string tmp , str1 , str2 ;
    str1 = eqexp->DumpStr(str0);
    str2 = relexp->DumpStr(str0);
    tmp = "%" + std::to_string(allsymTabCnt++);

    str0 += " ";
    str0 += tmp;
    str0 += " = ";   

    if(tag==EQ){
      str0 += "eq ";
      val = (eqexp->val==relexp->val);
    }
    else{
      str0 += "ne ";
      val = (eqexp->val!=relexp->val);
    }

    str0 += " ";
    str0 += str1;
    str0 += ", ";
    str0 += str2;
    str0 += "\n";

    // std::cout << str0 << std::endl;
    // std::cout << val << std::endl;
    return tmp;
  }
}

std::string LAndAST::DumpStr(std::string& str0) {
  // std::cout << "LAndAST" << std::endl;
  if(tag==EXP){
    std::string tmp = eqexp->DumpStr(str0);
    val = eqexp->val;
    std::cout << val << std::endl;
    return tmp;
  }
  else{
    std::string tmp1, tmp2, tmp3, lasttmp;
    int expIfCnt_ori = ifCnt;
    ifCnt++;
    std::string tmpreg = "@tttmp_" + std::to_string(expIfCnt_ori);
    str0 += " ";
    str0 += tmpreg;
    str0 += " = alloc i32\n";
    str0 += " store 0, " + tmpreg + "\n";

    tmp2 = landexp->DumpStr(str0);
    str0 += " %";
    str0 += std::to_string(allsymTabCnt++);
    str0 += " = ne 0, ";
    str0 += tmp2;
    str0 += "\n";

    str0 += " br %";
    str0 += std::to_string(allsymTabCnt-1);
    str0 += ", \%expthen_";
    str0 += std::to_string(expIfCnt_ori);
    str0 += ", \%expend_";
    str0 += std::to_string(expIfCnt_ori);
    str0 += "\n";

    str0 += "\%expthen_";
    str0 += std::to_string(expIfCnt_ori);
    str0 += ":\n";
    tmp3 = eqexp->DumpStr(str0);

    str0 += " %";
    str0 += std::to_string(allsymTabCnt++);
    str0 += " = ne 0, ";
    str0 += tmp3;
    str0 += "\n";

    str0 += " store %";
    str0 += std::to_string(allsymTabCnt-1);
    str0 += ", " + tmpreg + "\n";
    str0 += " jump \%expend_";
    str0 += std::to_string(expIfCnt_ori);
    str0 += "\n";

    str0 += "\%expend_";
    str0 += std::to_string(expIfCnt_ori);
    str0 += ":\n";

    tmp1 = "%" + std::to_string(allsymTabCnt++);
    lasttmp = tmp1;
    str0 += " ";
    str0 += tmp1;
    str0 += " = load " + tmpreg + "\n";

    if(landexp->val&&eqexp->val) val=1;
    else val=0;
    
    return tmp1;
  }
}

std::string LOrAST::DumpStr(std::string& str0) {
  // std::cout << "LOrAST" << std::endl;
  if(tag==EXP){
    std::string tmp = landexp->DumpStr(str0);
    val = landexp->val;
    std::cout << val << std::endl;
    return tmp;
  }
  else{
    std::string tmp1, tmp2, tmp3, lasttmp;
    int expIfCnt_ori = ifCnt;
    ifCnt++;
    std::string tmpreg = "@tttmp_" + std::to_string(expIfCnt_ori);
    str0 += " ";
    str0 += tmpreg;
    str0 += " = alloc i32\n";
    str0 += " store 1, " + tmpreg + "\n";

    tmp2 = lorexp->DumpStr(str0);
    str0 += " %";
    str0 += std::to_string(allsymTabCnt++);
    str0 += " = eq 0, ";
    str0 += tmp2;
    str0 += "\n";

    str0 += " br %";
    str0 += std::to_string(allsymTabCnt-1);
    str0 += ", \%expthen_";
    str0 += std::to_string(expIfCnt_ori);
    str0 += ", \%expend_";
    str0 += std::to_string(expIfCnt_ori);
    str0 += "\n";

    str0 += "\%expthen_";
    str0 += std::to_string(expIfCnt_ori);
    str0 += ":\n";
    tmp3 = landexp->DumpStr(str0);

    str0 += " %";
    str0 += std::to_string(allsymTabCnt++);
    str0 += " = ne 0, ";
    str0 += tmp3;
    str0 += "\n";

    str0 += " store %";
    str0 += std::to_string(allsymTabCnt-1);
    str0 += ", " + tmpreg + "\n";
    str0 += " jump \%expend_";
    str0 += std::to_string(expIfCnt_ori);
    str0 += "\n";

    str0 += "\%expend_";
    str0 += std::to_string(expIfCnt_ori);
    str0 += ":\n";

    tmp1 = "%" + std::to_string(allsymTabCnt++);
    lasttmp = tmp1;
    str0 += " ";
    str0 += tmp1;
    str0 += " = load " + tmpreg + "\n";

    if(landexp->val||lorexp->val) val=1;
    else val=0;

    return tmp1;
  }
}

std::string NumberAST::DumpStr(std::string& str0) {
  // std::cout << "NumberAST" << std::endl;
  return std::to_string(val);
} 

#endif