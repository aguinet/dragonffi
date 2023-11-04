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

// RUN: "%build_dir/anon_union%exeext" | "%FileCheck" "%s"

#include <iostream>
#include <dffi/dffi.h>

using namespace dffi;

typedef struct {
  char a;
  union {
    int a;
    short b;
  } u;
} A;

int main()
{
  DFFI::initialize();

  CCOpts Opts;
  Opts.OptLevel = 2;

  DFFI Jit(Opts);

  std::string Err;
  auto CU = Jit.compile(R"(
#include <stdio.h>
typedef struct {
  char a;
  union {
    int a;
    short b;
  } u;
} A;

void dump(A a) {
  printf("u.a=%d, u.b=%d\n", a.u.a, a.u.b);
}
)", Err);
  if (!CU) {
    std::cerr << Err << std::endl;
    return 1;
  }

  A a;
  a.u.a = 15;
  void* Args[] = { &a };
  // CHECK: u.a=15, u.b=15
  CU.getFunction("dump").call(&Args[0]);
  //Args[0] = &a.s;
  //CU.getFunction("dump_anon").call(&Args[0]);

  return 0;
}
