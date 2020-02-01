// Copyright 2018 Adrien Guinet <adrien@guinet.me>
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// RUN: "%build_dir/cconv%exeext"

#include <iostream>
#include <dffi/dffi.h>

using namespace dffi;

struct Res
{
  int a;
  int b;
  int res;
};

int main(int argc, char** argv)
{
  DFFI::initialize();

  CCOpts Opts;
  Opts.OptLevel = 2;

  DFFI Jit(Opts);

  std::string Err;
  auto CU = Jit.compile(R"(
__attribute__((ms_abi)) int add(int a) { return a+10; }
)", Err);

#define CHECK_COMPILE()\
  if (!CU) {\
    std::cerr << Err << std::endl;\
    return 1;\
  }
  CHECK_COMPILE()

  {
    int Arg = 1;
    int Ret;
    void* Args[] = {&Arg};
    CU.getFunction("add").call(&Ret, Args);
    if (Ret != 11) {
      std::cerr << "Invalid return value!" << std::endl;
      return 1;
    }
  }

  CU = Jit.compile(R"(
typedef struct
{
  int a;
  int b;
  int res;
} Res;

typedef Res(__attribute__((ms_abi)) *op)(int,int);

__attribute__((ms_abi)) static Res get_res(int res, int a, int b) {
  Res Ret = {a,b,res};
  return Ret;
}

__attribute__((ms_abi)) static Res add(int a, int b) {
  return get_res(a+b,a,b);
}
__attribute__((ms_abi)) static Res sub(int a, int b) {
  return get_res(a-b,a,b);
}

__attribute__((ms_abi)) op get_op(const char* name)
{
  if (name[0] == '+') return add;
  if (name[0] == '-') return sub;
  return 0;
}

__attribute__((ms_abi)) Res call(op f, int a, int b) {
  return f(a,b);
}
)", Err);
  CHECK_COMPILE()

  NativeFunc GetOp = CU.getFunction("get_op");
  PointerType const* PtrOpFTy = cast<PointerType>(GetOp.getReturnType());
  FunctionType const* OpFTy = cast<FunctionType>(PtrOpFTy->getPointee());

  void* AddOp;
  const char* OpName = "+";
  void* Args[] = {&OpName};
  GetOp.call(&AddOp, Args);

  void* SubOp;
  OpName = "-";
  Args[0] = &OpName;
  GetOp.call(&SubOp, Args);

  int a = 1;
  int b = 5;
  struct Res Ret;
  void* OpArgs[] = {&a, &b};

  Jit.getFunction(OpFTy, AddOp).call(&Ret, OpArgs);
  if (Ret.res != 6) {
    std::cerr << "add op failed!" << std::endl;
    return 1;
  }

  Jit.getFunction(OpFTy, SubOp).call(&Ret, OpArgs);
  if (Ret.res != -4) {
    std::cerr << "sub op failed!" << std::endl;
    return 1;
  }

  return 0;
}
