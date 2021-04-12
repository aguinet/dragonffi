// Copyright 2021 Adrien Guinet <adrien@guinet.me>
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

// RUN: "%build_dir/compile_cxx%exeext"

#include <iostream>
#include <cstdint>

#include <dffi/dffi.h>

using namespace dffi;

int main(int argc, char** argv)
{
  DFFI::initialize();

  CCOpts Opts;
  Opts.OptLevel = 2;
  Opts.CXX = CXXMode::Std11;

  DFFI Jit(Opts);

  std::string Err;
  auto CU = Jit.compile(R"(
    #include <cstdint>
    template <class T>
    static T foo(T a, T b) { return a+b; }
    extern "C" int32_t foo_int(int32_t a, int32_t b) { return foo(a,b); }
  )", Err);
  if (!CU) {
    std::cerr << "Compile error: " << Err << std::endl;
    return 1;
  }
  int32_t a = 2;
  int32_t b = 4;
  int32_t ret;
  void* Args_[] = {&a, &b};
  CU.getFunction("foo_int").call(&ret, &Args_[0]);
  if (ret != a+b) {
    std::cerr << "Invalid sum!" << std::endl;
    return 1;
  }

  return 0;
}
