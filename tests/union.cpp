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

// RUN: "%build_dir/union" >"%t"
// RUN: "%FileCheck" "%s" <"%t"

#include <stdio.h>
#include <dffi/dffi.h>
#include <dffi/composite_type.h>

using namespace dffi;

typedef union {
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
#include <stdio.h>
union A {
  int a;
  int b;
  short c;
  double d;
};

void dump(union A* a) {
  printf("a=%d, b=%d, c=%d\n", a->a, a->b, a->c);
}

void dump_value(union A a) {
  printf("a=%d, b=%d, c=%d\n", a.a, a.b, a.c);
}

void set(union A* a) {
  a->d = 4.;
}
)", Err);
  if (!CU) {
    fprintf(stderr, "%s\n", Err.c_str());
    return 1;
  }

  A obj;
  obj.a = 1;
  void* Args[] = {&obj};
  // CHECK: a=1, b=1, c=1
  CU.getFunction("dump_value").call(&Args[0]);
  A* pObj = &obj;
  Args[0] = &pObj;
  // CHECK: a=1, b=1, c=1
  CU.getFunction("dump").call(&Args[0]);
  CU.getFunction("set").call(&Args[0]);
  if (obj.d != 4.) {
    fprintf(stderr, "invalid value for d!\n", Err.c_str());
    return 1;
  }

  auto* UTy = CU.getUnionType("A");
  auto const& Fields = UTy->getFields();
  // CHECK: a
  // CHECK: b
  // CHECK: c
  // CHECK: d
  for (auto const& F: Fields) {
    printf("%s\n", F.getName());
  }

  return 0;
}
