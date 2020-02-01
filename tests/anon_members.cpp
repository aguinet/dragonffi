// Copyright 2019 Adrien Guinet <adrien@guinet.me>
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

// RUN: "%build_dir/anon_members%exeext"

#include <iostream>
#include <dffi/dffi.h>

using namespace dffi;

typedef struct {
  char buf[7];
  struct {
    int a;
    int b;
    struct {
      union {
        int c;
        float d;
      };
      int e;
    };
  };
  int f;
} A;

int main()
{
  DFFI::initialize();

  CCOpts Opts;
  Opts.OptLevel = 2;

  DFFI Jit(Opts);

  std::string Err;
  auto CU = Jit.compile(R"(
typedef struct {
  char buf[7];
  struct {
    int a;
    int b;
    struct {
      union {
        int c;
        float d;
      };
      int e;
    };
  };
  int f;
} A;

A foo() {
  A ret;
  ret.a = 1;
  ret.b = 2;
  ret.c = 4;
  ret.e = 5;
  ret.f = 6;
  return ret;
}
)", Err);
  if (!CU) {
    std::cerr << Err << std::endl;
    return 1;
  }

  A a;
  CU.getFunction("foo").call(&a, NULL);
  const bool Valid = (a.a == 1 && a.b == 2 && a.c == 4 && a.e == 5 && a.f == 6);
  return Valid?0:1;
}
