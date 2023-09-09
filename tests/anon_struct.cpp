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

// RUN: "%build_dir/anon_struct%exeext" | "%FileCheck" "%s"

#include <iostream>
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
#include <stdio.h>
typedef struct {
  char a;
  struct {
    int a;
    int b;
  } s;
} A;

void dump(A a) {
  printf("s.a=%d, s.b=%d\n", a.s.a, a.s.b);
}
)", Err);
  if (!CU) {
    std::cerr << Err << std::endl;
    return 1;
  }

  A a;
  a.s.a = 1; a.s.b = 2;
  void* Args[] = { &a };
  // CHECK: s.a=1, s.b=2
  CU.getFunction("dump").call(&Args[0]);

  // TODO: emit error in this case!
#if 0
  CU = Jit.compile(R"(
void dump_anon(struct { int a; int b; } s) {
  printf("a=%d, b=%d\n", s.a, s.b);
}
)", Err);
  if (!CU) {
    std::cerr << Err << std::endl;
    return 1;
  }
  Args[0] = &a.s;
  // IGNCHECK: a=1, b=2
  CU.getFunction("dump_anon").call(&Args[0]);

  CU = Jit.cdef(R"(
void dump_anon(struct { int a; int b; } s);
)", "api.h", Err);
  if (!CU) {
    std::cerr << Err << std::endl;
    return 1;
  }
  // IGNCHECK: a=1, b=2
  CU.getFunction("dump_anon").call(&Args[0]);
#endif
  return 0;
}
