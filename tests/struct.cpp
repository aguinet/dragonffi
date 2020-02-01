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

// RUN: "%build_dir/struct%exeext" | "%FileCheck" "%s"

#include <iostream>
#include <dffi/dffi.h>

using namespace dffi;

typedef struct {
  int a;
  int b;
  short c;
  double d;
} A;

int main()
{
  DFFI::initialize();

  CCOpts Opts;
  Opts.OptLevel = 2;

  DFFI Jit(Opts);

  std::string Err;
  auto CU = Jit.compile(R"(
struct A {
  int a;
  int b;
  short c;
  double d;
};

void foo2(struct A* a);

void foo(struct A a) {
  foo2(&a);
}

void foo2(struct A* a) {
  printf("a=%d, b=%d, c=%d, d=%f\n", a->a, a->b, a->c, a->d);
  a->a = 2;
}

void set(struct A* a) {
  a->d = 4.;
}
)", Err);
  if (!CU) {
    std::cerr << Err << std::endl;
    return 1;
  }

  A obj = {1,2,4,5.};
  void* Args[] = {&obj};
  // CHECK: a=1, b=2, c=4, d=5.000000
  CU.getFunction("foo").call(&Args[0]);
  if (obj.a != 1) {
    std::cerr << "invalid value for a!" << std::endl;
    return 1;
  }
  A* pObj = &obj;
  Args[0] = &pObj;
  // CHECK: a=1, b=2, c=4, d=5.000000
  CU.getFunction("foo2").call(&Args[0]);
  if (obj.a != 2) {
    std::cerr << "invalid value for a!" << std::endl;
    return 1;
  }
  CU.getFunction("set").call(&Args[0]);
  if (obj.d != 4.) {
    std::cerr << "invalid value for d!" << std::endl;
    return 1;
  }

  return 0;
}
