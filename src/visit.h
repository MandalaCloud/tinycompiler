#ifndef VISIT_H
#define VISIT_H

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
#include "op.h"
#include "koopa.h"
#define DEBUGMODE 0
using namespace std;

string restr;
map<koopa_raw_value_t, int> stackForInsts;
map<koopa_raw_function_t, int> mapFuncToSp;
map<koopa_raw_function_t, int> mapFuncToRa;
koopa_raw_function_t curFunc;

void Visit(const koopa_raw_program_t &program);
void Visit(const koopa_raw_slice_t &slice);
void Visit(const koopa_raw_function_t &func);
void Visit(const koopa_raw_basic_block_t &bb);
void Visit(const koopa_raw_value_t &value);
void Visit(const koopa_raw_return_t &ret);
void Visit(const koopa_raw_integer_t &integer);
void Visit(const koopa_raw_binary_t &binary);
void Visit(const koopa_raw_store_t &rawStore);
void Visit(const koopa_raw_load_t &load);
void Visit(const koopa_raw_jump_t &jump);
void Visit(const koopa_raw_branch_t &branch);
void Visit(const koopa_raw_call_t &call);
void Visit(const koopa_raw_func_arg_ref_t &funcArgRef) {}
void Visit(const koopa_raw_global_alloc_t &myGlobalAlloc);
void Visit(const koopa_raw_aggregate_t &myAggregate);
void Visit(const koopa_raw_get_elem_ptr_t &myGetElemPtr);
void Visit(const koopa_raw_get_ptr_t &myGetPtr);

void myEpilogue(const koopa_raw_function_t &func);
void myPrologue(const koopa_raw_function_t &func);

void myPrintGlobalVar(const koopa_raw_value_t &value);
void myInitArray(const koopa_raw_value_t &value);

void safeSw(string src, string offset, string addr);

void safeLw(string dest, string offset, string addr);

void safeSw(string src, int offset, string addr)
{
  if (offset >= -2048 && offset <= 2047)
    restr += " sw " + src + ", " + to_string(offset) + "(" + addr + ")\n";
  else
  {
    restr += " li t6, " + to_string(offset) + "\n";
    restr += " add t5, t6, " + addr + "\n";
    restr += " sw " + src + ", 0(t5)\n";
  }
}

void safeLw(string dest, int offset, string addr)
{
  if (offset >= -2048 && offset <= 2047)
    restr += " lw " + dest + ", " + to_string(offset) + "(" + addr + ")\n";
  else
  {
    restr += " li t6, " + to_string(offset) + "\n";
    restr += " add t5, t6, " + addr + "\n";
    restr += " lw " + dest + ", 0(t5)\n";
  }
}

void writeTo(const koopa_raw_value_t &value, string srcReg)
{
  if (stackForInsts.find(value) == stackForInsts.end())
  {
    if (value->kind.tag == KOOPA_RVT_GLOBAL_ALLOC)
    {
      string myGlobalName = value->name;
      myGlobalName.erase(0, 1);
      restr += " la t1, " + myGlobalName + "\n";
      restr += " sw t0, 0(t1)\n";
    }
  }
  else
  {
    safeSw(srcReg, stackForInsts[value], "sp");
    restr += "\n";
  }
}

void readFrom(const koopa_raw_value_t &value, string destReg)
{
  if (stackForInsts.find(value) != stackForInsts.end())
    safeLw(destReg, stackForInsts[value], "sp");
  else if (value->kind.tag == KOOPA_RVT_INTEGER)
    restr += " li " + destReg + ", " + to_string(value->kind.data.integer.value) + "\n";
  else if (value->kind.tag == KOOPA_RVT_GLOBAL_ALLOC)
  {
    string myGlobalName = value->name;
    myGlobalName.erase(0, 1);
    restr += " la " + destReg + ", " + myGlobalName + "\n";
    restr += " lw " + destReg + ", 0(" + destReg + ")\n";
  }
}

void readAddrFrom(const koopa_raw_value_t &value, string destReg)
{
  if (stackForInsts.find(value) != stackForInsts.end())
    restr += " addi " + destReg + ", " + "sp ," + to_string(stackForInsts[value]) + "\n";
  else if (value->kind.tag == KOOPA_RVT_GLOBAL_ALLOC)
  {
    string myGlobalName = value->name;
    myGlobalName.erase(0, 1);
    restr += " la " + destReg + ", " + myGlobalName + "\n";
  }
}

void Visit(const koopa_raw_program_t &program)
{
  for (size_t i = 0; i < program.values.len; ++i)
    assert(program.values.kind == KOOPA_RSIK_VALUE);

  for (size_t i = 0; i < program.funcs.len; ++i)
    assert(program.funcs.kind == KOOPA_RSIK_FUNCTION);

  Visit(program.values);

  Visit(program.funcs);
}

// 访问 raw slice
void Visit(const koopa_raw_slice_t &slice)
{
  for (size_t i = 0; i < slice.len; ++i)
  {
    auto ptr = slice.buffer[i];
    // 根据 slice 的 kind 决定将 ptr 视作何种元素
    switch (slice.kind)
    {
    case KOOPA_RSIK_FUNCTION:
      // 访问函数
      if (reinterpret_cast<koopa_raw_function_t>(ptr)->bbs.len != 0)
        Visit(reinterpret_cast<koopa_raw_function_t>(ptr));
      break;
    case KOOPA_RSIK_BASIC_BLOCK:
      // 访问基本块
      Visit(reinterpret_cast<koopa_raw_basic_block_t>(ptr));
      break;
    case KOOPA_RSIK_VALUE:
      // 访问指令
      Visit(reinterpret_cast<koopa_raw_value_t>(ptr));
      break;
    default:
      // 我们暂时不会遇到其他内容, 于是不对其做任何处理
      assert(false);
    }
  }
}

// 访问函数
void Visit(const koopa_raw_function_t &func)
{
  // 执行一些其他的必要操作
  // ...
  restr += "  .text \n";
  restr += "  .global ";
  string funcName = func->name;
  funcName.erase(0, 1);
  restr += funcName + "\n";
  restr += funcName;
  restr += ":\n";
  // 访问所有基本块
  Visit(func->params);

  int lenForBbs = func->bbs.len;
  // cout<<"lenForBbs "<<lenForBbs<<endl;
  int spForEachInst = 0;
  int spForRa = 0;
  int spForFuncParams = 0;
  int maxLenForArgs = 0;
  for (int i = 0; i < lenForBbs; i++)
  {
    auto ptr = func->bbs.buffer[i];
    koopa_raw_basic_block_t funcBb = reinterpret_cast<koopa_raw_basic_block_t>(ptr);
    int lenForInsts = funcBb->insts.len;
    for (int j = 0; j < lenForInsts; j++)
    {
      auto ptr = funcBb->insts.buffer[j];
      koopa_raw_value_t bbInst = reinterpret_cast<koopa_raw_value_t>(ptr);
      if (bbInst->ty->tag != KOOPA_RTT_UNIT)
      {
        if (bbInst->ty->tag == KOOPA_RTT_POINTER && bbInst->kind.tag == KOOPA_RVT_ALLOC && bbInst->ty->data.pointer.base->tag == KOOPA_RTT_ARRAY)
        {
          auto targetArray = bbInst->ty->data.pointer.base->data.array;
          int arraySize = targetArray.len;
          while (targetArray.base->tag == KOOPA_RTT_ARRAY)
          {
            targetArray = targetArray.base->data.array;
            arraySize *= targetArray.len;
          }
          arraySize *= 4;
          spForEachInst += arraySize;
        }
        else
          spForEachInst += 4;
      }
      if (bbInst->kind.tag == KOOPA_RVT_CALL)
      {
        if (spForRa == 0)
          spForRa = 4;
        koopa_raw_slice_t funcArgs = bbInst->kind.data.call.args;
        int lenForFuncArgs = funcArgs.len;
        maxLenForArgs = max(lenForFuncArgs, maxLenForArgs);
      }
    }
  }
  spForFuncParams = max(0, maxLenForArgs - 8) * 4;
  int spForAll = spForEachInst + spForFuncParams + spForRa;
  spForAll = (spForAll + 15) / 16 * 16;

  mapFuncToSp[func] = spForAll;
  mapFuncToRa[func] = spForRa;
  int specialSpForEachInst = 0;
  for (int i = 0; i < lenForBbs; i++)
  {
    auto ptr = func->bbs.buffer[i];
    koopa_raw_basic_block_t funcBb = reinterpret_cast<koopa_raw_basic_block_t>(ptr);
    int lenForInsts = funcBb->insts.len;
    for (int j = 0; j < lenForInsts; j++)
    {
      auto ptr = funcBb->insts.buffer[j];
      koopa_raw_value_t bbInst = reinterpret_cast<koopa_raw_value_t>(ptr);
      if (bbInst->ty->tag != KOOPA_RTT_UNIT)
      {
        stackForInsts[bbInst] = spForFuncParams + specialSpForEachInst;
        if (bbInst->ty->tag == KOOPA_RTT_POINTER && bbInst->kind.tag == KOOPA_RVT_ALLOC && bbInst->ty->data.pointer.base->tag == KOOPA_RTT_ARRAY)
        {
          auto targetArray = bbInst->ty->data.pointer.base->data.array;
          int arraySize = targetArray.len;
          while (targetArray.base->tag == KOOPA_RTT_ARRAY)
          {
            targetArray = targetArray.base->data.array;
            arraySize *= targetArray.len;
          }
          arraySize *= 4;
          specialSpForEachInst += arraySize;
        }
        else
          specialSpForEachInst += 4;
      }
    }
  }
  koopa_raw_function_t prevFunc = curFunc;
  curFunc = func;
  myPrologue(curFunc);
  Visit(func->bbs);
  curFunc = prevFunc;
}

void myPrologue(const koopa_raw_function_t &func)
{
  int sp = mapFuncToSp[func];
  int ra = mapFuncToRa[func];
  if (sp != 0)
  {
    if ((-sp) >= -2048 && (-sp) <= 2047)
      restr += " addi sp, sp, " + to_string(-sp) + "\n";
    else
    {
      restr += " li t6, " + to_string(-sp) + "\n";
      restr += " add sp, sp, t6\n";
    }

    if (ra != 0)
    {
      if ((sp - 4) >= -2048 && (sp - 4) <= 2047)
        restr += " sw ra, " + to_string(sp - 4) + "(sp)\n";
      else
      {
        restr += " li t6, " + to_string(sp - 4) + "\n";
        restr += " add t5, t6, sp\n";
        restr += " sw ra, 0(t5)\n";
      }
    }
  }
}

void myEpilogue(const koopa_raw_function_t &func)
{
  int sp = mapFuncToSp[func];
  int ra = mapFuncToRa[func];
  if (sp != 0)
  {
    if (ra != 0)
    {
      if ((sp - 4) >= -2048 && (sp - 4) <= 2047)
        restr += " lw ra, " + to_string(sp - 4) + "(sp)\n";
      else
      {
        restr += " li t6, " + to_string(sp - 4) + "\n";
        restr += " add t5, t6, sp\n";
        restr += " lw ra, 0(t5)\n";
      }
    }
    int tmpoffset = mapFuncToSp[func];
    if (tmpoffset >= -2048 && tmpoffset <= 2047)
      restr += " addi sp, sp, " + to_string(mapFuncToSp[func]) + "\n";
    else
    {
      restr += " li t6, " + to_string(tmpoffset) + "\n";
      restr += " add sp, sp, t6\n";
    }
  }
}

// 访问基本块
void Visit(const koopa_raw_basic_block_t &bb)
{
  // 执行一些其他的必要操作
  // ...
  // 访问所有指令
  string bbLabel = bb->name;
  bbLabel.erase(0, 1);
  if (bbLabel != "entry")
    restr += bbLabel + ":\n";
  Visit(bb->insts);
}

// 访问指令
void Visit(const koopa_raw_value_t &value)
{
  // 根据指令类型判断后续需要如何访问
  const auto &kind = value->kind;
  cout << kind.tag << endl;
  switch (kind.tag)
  {
  case KOOPA_RVT_RETURN:
    // 访问 return 指令
    Visit(kind.data.ret);
    break;
  case KOOPA_RVT_INTEGER:
    // 访问 integer 指令
    Visit(kind.data.integer);
    break;
  case KOOPA_RVT_STORE:
    // 访问 integer 指令
    // cout<<"KOOPA_RVT_STORE"<<endl;
    Visit(kind.data.store);
    // writeTo(value);
    break;
  case KOOPA_RVT_LOAD:
    Visit(kind.data.load);
    writeTo(value, "t0");
    break;
  case KOOPA_RVT_BINARY:
    // cout<<"KOOPA_RVT_BINARY"<<endl;
    Visit(kind.data.binary);
    writeTo(value, "t0");
    break;
  case KOOPA_RVT_ALLOC:

    // cout<<"KOOPA_RVT_ALLOC"<<endl;

    break;
  case KOOPA_RVT_BRANCH:
    Visit(kind.data.branch);
    break;
  case KOOPA_RVT_JUMP:
    Visit(kind.data.jump);
    break;
  case KOOPA_RVT_CALL:
    Visit(kind.data.call);
    writeTo(value, "a0");
    break;
  case KOOPA_RVT_FUNC_ARG_REF:
    Visit(kind.data.func_arg_ref);
    break;
  case KOOPA_RVT_GLOBAL_ALLOC:
    myPrintGlobalVar(value);
    Visit(kind.data.global_alloc);
    break;
  case KOOPA_RVT_ZERO_INIT:
    myInitArray(value);
    break;
  case KOOPA_RVT_AGGREGATE:
    Visit(kind.data.aggregate);
    break;
  case KOOPA_RVT_GET_ELEM_PTR:
    Visit(kind.data.get_elem_ptr);
    writeTo(value, "t0");
    // assert(false);
    break;
  case KOOPA_RVT_GET_PTR:
    Visit(kind.data.get_ptr);
    writeTo(value, "t0");
    break;
  default:
    // 其他类型暂时遇不到
    cout << kind.tag << endl;
    assert(false);
  }
}

void myPrintGlobalVar(const koopa_raw_value_t &value)
{
  restr += " .data\n";
  string myGlobalName = value->name;
  myGlobalName.erase(0, 1);
  restr += " .global " + myGlobalName + "\n";
  restr += myGlobalName + ":\n";
}

void myInitArray(const koopa_raw_value_t &value)
{
  restr += " .zero ";
  if (value->ty->tag == KOOPA_RTT_INT32)
    restr += "4";
  else if (value->ty->tag == KOOPA_RTT_ARRAY)
  {
    auto curArray = value->ty->data.array;
    int sizeOfArray = curArray.len;
    while (curArray.base->tag == KOOPA_RTT_ARRAY)
    {
      sizeOfArray *= curArray.base->data.array.len;
      curArray = curArray.base->data.array;
    }
    sizeOfArray *= 4;
    restr += to_string(sizeOfArray);
  }

  restr += "\n";
}
// 访问对应类型指令的函数定义略
// 视需求自行实现
// ...

void Visit(const koopa_raw_return_t &ret)
{
  if (ret.value == NULL)
  {
  }
  else if (stackForInsts.find(ret.value) != stackForInsts.end())
    safeLw("a0", stackForInsts[ret.value], "sp");
  else
  {
    restr += " li a0, ";
    restr += to_string(ret.value->kind.data.integer.value);
    restr += "\n";
  }

  myEpilogue(curFunc);
  restr += " ret\n";
}

void Visit(const koopa_raw_call_t &call)
{
  auto callFuncArgs = call.args;
  int lenForCallFuncArgs = callFuncArgs.len;
  for (int i = 0; i < lenForCallFuncArgs; i++)
  {
    string destReg = "";
    if (i < 8)
    {
      destReg = "a" + to_string(i);
      readFrom(reinterpret_cast<koopa_raw_value_t>(call.args.buffer[i]), destReg);
    }
    else
    {
      string destMem = to_string((i - 8) * 4) + "(sp)";
      readFrom(reinterpret_cast<koopa_raw_value_t>(call.args.buffer[i]), "t0");
      restr += "sw t0, " + destMem + "\n";
    }
  }
  string callFuncName = call.callee->name;
  callFuncName.erase(0, 1);
  restr += " call " + callFuncName + "\n";
}

void Visit(const koopa_raw_global_alloc_t &myGlobalAlloc)
{
  if (myGlobalAlloc.init->kind.tag == KOOPA_RVT_INTEGER)
    restr += " .word " + to_string(myGlobalAlloc.init->kind.data.integer.value) + "\n";
  else if (myGlobalAlloc.init->kind.tag == KOOPA_RVT_ZERO_INIT)
    Visit(myGlobalAlloc.init);
  else if (myGlobalAlloc.init->kind.tag == KOOPA_RVT_AGGREGATE)
    Visit(myGlobalAlloc.init);
}

void Visit(const koopa_raw_aggregate_t &myAggregate)
{
  Visit(myAggregate.elems);
}

void Visit(const koopa_raw_get_elem_ptr_t &myGetElemPtr)
{
  if (myGetElemPtr.src->ty->tag != KOOPA_RTT_POINTER)
  {
    assert(false);
  }
  auto srcPtr = myGetElemPtr.src->ty->data.pointer;
  int sizeOfPtr = 1;
  if (srcPtr.base->tag == KOOPA_RTT_INT32)
    sizeOfPtr *= 4;
  else
  {
    auto srcArray = srcPtr.base->data.array;
    while (srcArray.base->tag != KOOPA_RTT_INT32)
    {
      if (srcArray.base->tag != KOOPA_RTT_ARRAY)
      {
        assert(false);
      }
      sizeOfPtr *= srcArray.base->data.array.len;
      srcArray = srcArray.base->data.array;
    }
    sizeOfPtr *= 4;
  }

  if (myGetElemPtr.src->kind.tag == KOOPA_RVT_ALLOC)
    readAddrFrom(myGetElemPtr.src, "t0");
  else if (myGetElemPtr.src->kind.tag == KOOPA_RVT_GLOBAL_ALLOC)
    readAddrFrom(myGetElemPtr.src, "t0");
  else
    readFrom(myGetElemPtr.src, "t0");

  readFrom(myGetElemPtr.index, "t1");

  restr += " li t2, " + to_string(sizeOfPtr) + '\n';
  restr += " mul t1, t1, t2\n";
  restr += " add t0, t0, t1\n";
}

void Visit(const koopa_raw_get_ptr_t &myGetPtr)
{
  if (myGetPtr.src->ty->tag != KOOPA_RTT_POINTER)
  {
    assert(false);
  }
  auto srcPtr = myGetPtr.src->ty->data.pointer;
  int sizeOfPtr = 1;
  if (srcPtr.base->tag == KOOPA_RTT_INT32)
    sizeOfPtr *= 4;
  else
  {
    auto srcArray = srcPtr.base->data.array;
    while (srcArray.base->tag != KOOPA_RTT_INT32)
    {
      if (srcArray.base->tag != KOOPA_RTT_ARRAY)
      {
        assert(false);
      }
      sizeOfPtr *= srcArray.base->data.array.len;
      srcArray = srcArray.base->data.array;
    }
    sizeOfPtr *= 4;
  }

  if (myGetPtr.src->kind.tag == KOOPA_RVT_ALLOC)
    readAddrFrom(myGetPtr.src, "t0");
  else if (myGetPtr.src->kind.tag == KOOPA_RVT_GLOBAL_ALLOC)
    readAddrFrom(myGetPtr.src, "t0");
  else
    readFrom(myGetPtr.src, "t0");

  readFrom(myGetPtr.index, "t1");

  restr += " li t2, " + to_string(sizeOfPtr) + '\n';
  restr += " mul t1, t1, t2\n";
  restr += " add t0, t0, t1\n";
}

void Visit(const koopa_raw_branch_t &branch)
{
  readFrom(branch.cond, "t0");
  string trueLabel = branch.true_bb->name;
  trueLabel.erase(0, 1);
  restr += " bnez t0, " + trueLabel + "\n";
  string falseLabel = branch.false_bb->name;
  falseLabel.erase(0, 1);
  restr += " j " + falseLabel + "\n";
}

void Visit(const koopa_raw_jump_t &jump)
{
  string targetLabel = jump.target->name;
  targetLabel.erase(0, 1);
  restr += " j " + targetLabel + "\n";
}

void Visit(const koopa_raw_binary_t &binary)
{

  readFrom(binary.lhs, "t0");
  readFrom(binary.rhs, "t1");

  switch (binary.op)
  {
  case KOOPA_RBO_EQ:
    restr += " xor t0, t0, t1\n";
    restr += " seqz t0, t0\n";
    break;
  case KOOPA_RBO_NOT_EQ:
    restr += " xor t0, t0, t1\n";
    restr += " snez t0, t0\n";
    break;
  case KOOPA_RBO_GT:
    restr += " sgt t0, t0, t1\n";
    break;
  case KOOPA_RBO_LT:
    restr += " slt t0, t0, t1\n";
    break;
  case KOOPA_RBO_GE:
    restr += " slt t0, t0, t1\n";
    restr += " seqz t0, t0\n";
    break;
  case KOOPA_RBO_LE:
    restr += " sgt t0, t0, t1\n";
    restr += " seqz t0, t0\n";
    break;
  case KOOPA_RBO_SUB:
    restr += " sub t0, t0, t1\n";
    break;
  case KOOPA_RBO_ADD:
    restr += " add t0, t0, t1\n";
    break;
  case KOOPA_RBO_MUL:
    restr += " mul t0, t0, t1\n";
    break;
  case KOOPA_RBO_DIV:
    restr += " div t0, t0, t1\n";
    break;
  case KOOPA_RBO_MOD:
    restr += " rem t0, t0, t1\n";
    break;
  case KOOPA_RBO_AND:
    restr += " and t0, t0, t1\n";
    break;
  case KOOPA_RBO_OR:
    restr += " or t0, t0, t1\n";
    break;
  case KOOPA_RBO_XOR:
    restr += " xor t0, t0, t1\n";
    break;
  case KOOPA_RBO_SHL:
    restr += " sll t0, t0, t1\n";
    break;
  case KOOPA_RBO_SHR:
    restr += " srl t0, t0, t1\n";
    break;
  case KOOPA_RBO_SAR:
    restr += " sra t0, t0, t1\n";
    break;
  default:
    assert(false);
  }
}

void Visit(const koopa_raw_store_t &rawStore)
{
  if (rawStore.value->kind.tag == KOOPA_RVT_FUNC_ARG_REF)
  {
    int myIndex = rawStore.value->kind.data.func_arg_ref.index;
    if (myIndex < 8)
      writeTo(rawStore.dest, "a" + to_string(rawStore.value->kind.data.func_arg_ref.index));
    else
    {
      int spNumForCalleeArgs = mapFuncToSp[curFunc] + (myIndex - 8) * 4;
      restr += " lw t0, " + to_string(spNumForCalleeArgs) + "(sp)\n";
      writeTo(rawStore.dest, "t0");
    }
  }
  else
  {
    readFrom(rawStore.value, "t0");
    if (rawStore.dest->ty->tag == KOOPA_RTT_POINTER && (rawStore.dest->kind.tag == KOOPA_RVT_GET_ELEM_PTR || rawStore.dest->kind.tag == KOOPA_RVT_GET_PTR))
    {
      readFrom(rawStore.dest, "t1");
      restr += " sw t0, 0(t1)\n";
    }
    else
      writeTo(rawStore.dest, "t0");
  }
}

void Visit(const koopa_raw_load_t &load)
{
  readFrom(load.src, "t0");
  if (load.src->ty->tag == KOOPA_RTT_POINTER && (load.src->kind.tag == KOOPA_RVT_GET_ELEM_PTR || load.src->kind.tag == KOOPA_RVT_GET_PTR))
    restr += " lw t0, 0(t0)\n";
}

void Visit(const koopa_raw_integer_t &integer)
{
  restr += " .word " + to_string(integer.value) + '\n';
}

#endif