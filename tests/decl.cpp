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

// RUN: "%build_dir/decl%exeext" | "%FileCheck" "%s"

#include <iostream>
#include <dffi/dffi.h>

using namespace dffi;

int main(int argc, char** argv)
{
  DFFI::initialize();

  CCOpts Opts;
  Opts.OptLevel = 2;
  Opts.LazyJITWrappers = false;

  DFFI Jit(Opts);

  const char* myinc;
#ifdef _WIN32
  myinc = nullptr;
#else
  myinc = "/myinc.h";
#endif
  std::string Err;
  auto CU = Jit.cdef(R"(
void print(const char* s);
static const char* msg="def msg";
)",
    myinc, Err);

#define CHECK_COMPILE()\
  if (!CU) {\
    std::cerr << Err << std::endl;\
    return 1;\
  }
  CHECK_COMPILE()

  CU = Jit.cdef(R"(
#include <stdio.h>
void print(const char* s)
{
  puts(s);
}
)", nullptr, Err);
  CHECK_COMPILE()

  const char* arg0 = "coucou";
  void* Args[] = {&arg0};
  int Ret;
  // CHECK: coucou
  CU.getFunction("puts").call(&Ret, Args);
  // CHECK: coucou
  CU.getFunction("puts").call(&Ret, Args);

  CU = Jit.compile(R"(
#ifdef _WIN32
// Includes of previously defined CU is broken under Windows...
void print(const char* s);
static const char* msg="def msg";
#else
#include "/myinc.h"
#endif
void foo() {
  print(msg);
  print("coucou from foo");
}

void toto()
{
  print("coucou from toto");
})", Err);
  CHECK_COMPILE()

  // CHECK: coucou from toto
  CU.getFunction("toto").call(nullptr);

  // CHECK: def msg
  // CHECK: coucou from foo
  CU.getFunction("foo").call(nullptr);

  CU = Jit.compile(R"(
void print(const char* s);
void foo();
void foo2() {
  print("foo2");
  foo();
})", Err);
  CHECK_COMPILE()

  // CHECK: foo2
  // CHECK: def msg
  // CHECK: coucou from foo
  CU.getFunction("foo2").call(nullptr);

  CU = Jit.cdef(R"(
#include <stdint.h>
typedef uint32_t MyInt;
)", nullptr, Err);
  CHECK_COMPILE()
  Type const* Ty = CU.getType("MyInt");
  if (!Ty) {
    std::cerr << "type MyInt isn't defined!" << std::endl;
    return 1;
  }
  BasicType const* BTy = dyn_cast<BasicType>(Ty);
  if (!BTy) {
    std::cerr << "type MyInt isn't a basic type!" << std::endl;
    return 1;
  }
  if (BTy->getBasicKind() != BasicType::getKind<uint32_t>()) {
    std::cerr << "type MyInt isn't an uint32_t!" << std::endl;
    return 1;
  }

  return 0;
}
