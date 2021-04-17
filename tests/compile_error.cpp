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

// RUN: "%build_dir/compile_error%exeext" 1>"%t.out" 2>"%t.err"
// RUN: "%FileCheck" "%s" <"%t.out"
// RUN: "%count" 0 <"%t.err"

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
  // CHECK: error: unknown type name 'this'
  if (Jit.compile("this is not valid C code", Err)) {
    // We want an error, so returning 1 in the case no error is found!
    return 1;
  }

  std::cout << Err << std::endl;

  // CHECK-NOT: error: unknown type name 'this'
  // CHECK-DAG: error: unknown type name 'still'
  if (Jit.compile("still invalide code", Err)) {
    return 1;
  }

  std::cout << Err << std::endl;

  if (!Jit.compile("int foo(int a, int b) { return a+b; }", Err)) {
    std::cout << "error!! " << Err << std::endl;
    return 1;
  }

  return 0;
}
