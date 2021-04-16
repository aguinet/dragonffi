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

// RUN: "%build_dir/varargs%exeext" | "%FileCheck" "%s"

#include <cstdio>
#include <dffi/dffi.h>

using namespace dffi;

typedef struct {
  char a;
  struct {
    int a;
    int b;
  } s;
} A;

int main()
{
  DFFI::initialize();

  CCOpts Opts;
  Opts.OptLevel = 2;
  Opts.LazyJITWrappers = false;

  DFFI Jit(Opts);

  std::string Err;
  auto CU = Jit.compile(R"(
#include <stdarg.h>
#include <stdio.h>

void print(const char* prefix, ...)
{
  va_list args;
  va_start(args, prefix);
  while (1) {
    const char* str = va_arg(args, const char*);
    if (!str) break;
    printf("%s: %s\n", prefix, str);
  }
  va_end(args);
}
)", Err);
  if (!CU) {
    fprintf(stderr, "%s\n", Err.c_str());
    return 1;
  }

  const void* Args[4];
  void* NullPtr = nullptr;
  Type const* VarArgsTy[3];
  const char* Prefix = "prefix";
  const char* Str0 = "coucou";
  Args[0] = &Prefix;
  Args[1] = &Str0;
  Args[2] = &NullPtr;
  VarArgsTy[0] = Jit.getCharPtrTy();
  VarArgsTy[1] = Jit.getCharPtrTy();
  VarArgsTy[2] = Jit.getCharPtrTy();
  // CHECK: prefix: coucou
  CU.getFunction("print", VarArgsTy, 2).call((void**)Args);
  // CHECK: prefix: coucou
  CU.getFunction("print", VarArgsTy, 2).call((void**)Args);

  const char* Str1 = "coucou2";
  Args[2] = &Str1;
  Args[3] = &NullPtr;
  // CHECK: prefix: coucou
  // CHECK: prefix: coucou2
  CU.getFunction("print", VarArgsTy, 3).call((void**)Args);

  CU = Jit.cdef(R"(
void print(const char* prefix, ...);
)", nullptr, Err);
  if (!CU) {
    fprintf(stderr, "%s\n", Err.c_str());
    return 1;
  }
  // CHECK: prefix: coucou
  // CHECK: prefix: coucou2
  CU.getFunction("print", VarArgsTy, 3).call((void**)Args);

  return 0;
}
