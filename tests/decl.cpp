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

// RUN: "%build_dir/decl" | "%FileCheck" "%s"

#include <iostream>
#include <dffi/dffi.h>

using namespace dffi;

int main(int argc, char** argv)
{
  DFFI::initialize();

  CCOpts Opts;
  Opts.OptLevel = 2;

  DFFI Jit(Opts);

  std::string Err;
  auto CU = Jit.cdef(R"(
void puts(const char* s);
static const char* msg="def msg";
)",
    "/stdio.h", Err);

#define CHECK_COMPILE()\
  if (!CU) {\
    std::cerr << Err << std::endl;\
    return 1;\
  }
  CHECK_COMPILE()

  const char* arg0 = "coucou";
  void* Args[] = {&arg0};
  // CHECK: coucou
  CU.getFunction("puts").call(Args);
  // CHECK: coucou
  CU.getFunction("puts").call(Args);

  CU = Jit.compile(R"(
#include "/stdio.h"
void foo() {
  puts(msg);
  puts("coucou from foo");
}

void toto()
{
  puts("coucou from toto");
})", Err);
  CHECK_COMPILE()

  // CHECK: coucou from toto
  CU.getFunction("toto").call(nullptr);

  // CHECK: def msg
  // CHECK: coucou from foo
  CU.getFunction("foo").call(nullptr);

  CU = Jit.compile(R"(
void foo();
void foo2() {
  puts("foo2");
  foo();
})", Err);
  CHECK_COMPILE()

  // CHECK: foo2
  // CHECK: def msg
  // CHECK: coucou from foo
  CU.getFunction("foo2").call(nullptr);

  return 0;
}
