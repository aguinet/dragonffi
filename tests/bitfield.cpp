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

// RUN: "%build_dir/bitfield"

#include <iostream>
#include <dffi/dffi.h>

using namespace dffi;

typedef struct {
  int a: 1;
  int b: 5;
  int c: 4;
  int d: 6;
  int e: 16;
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
  int a: 1;
  int b: 5;
  int c: 4;
  int d: 6;
  int e: 16;
};

void foo2(struct A* a);

void foo(struct A a) {
  foo2(&a);
}

void foo2(struct A* a) {
  printf("a=%d, b=%d, c=%d, d=%d, e=%d\n", a->a, a->b, a->c, a->d, a->e);
}
)", Err);
  if (!CU) {
    std::cerr << Err << std::endl;
    return 1;
  }

  A obj = {1,2,4,5,6};
  printf("a=%d, b=%d, c=%d, d=%d, e=%d\n", obj.a, obj.b, obj.c, obj.d, obj.e);
  void* Args[] = {&obj};
  CU.getFunction("foo").call(&Args[0]);

  return 0;
}
