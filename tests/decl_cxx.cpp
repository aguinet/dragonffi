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

// RUN: "%build_dir/decl_cxx%exeext"

#include <iostream>
#include <set>

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
  auto CU = Jit.cdef(R"(
extern "C" void print(const char* s);
extern "C" {
  int foo();
  int bar();
}
)",
    nullptr, Err);

#define CHECK_COMPILE()\
  if (!CU) {\
    std::cerr << Err << std::endl;\
    return 1;\
  }
  CHECK_COMPILE()

  const auto funcs = CU.getFunctions();
  const std::set<std::string> funcsSet(funcs.begin(), funcs.end());

#define CHECK_FUNC(name) \
  if (funcsSet.find(name) == funcsSet.end()) { \
    std::cerr << "missing " << name << std::endl; \
    return 1; \
  }

  CHECK_FUNC("print");
  CHECK_FUNC("foo");
  CHECK_FUNC("bar");

  return 0;
}
