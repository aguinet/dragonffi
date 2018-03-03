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

// RUN: "%build_dir/inline"

#include <iostream>
#include <dffi/dffi.h>

using namespace dffi;

int main()
{
  DFFI::initialize();

  CCOpts Opts;
  Opts.OptLevel = 2;

  DFFI Jit(Opts);

  std::string Err;
  auto CU = Jit.cdef(R"(
inline int foo() { return 1; }
int bar() { return foo(); }
)", nullptr, Err);
  if (!CU) {
    std::cerr << Err << std::endl;
    return 1;
  }

  Jit.compile("int foo() { return 1; }", Err);

  NativeFunc F = CU.getFunction("foo");
  if (!F) {
    std::cerr << "foo isn't available!" << std::endl;
    return 1;
  }

  return 0;
}
