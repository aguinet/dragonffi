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

// RUN: "%build_dir/stdint"

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
  auto CU = Jit.compile(R"(
#include <stdint.h>
uint32_t foo(uint32_t a) {
  return a+1;
}
)", Err);
  if (!CU) {
    std::cerr << Err << std::endl;
    return 1;
  }

  auto const* STy = CU.getType("uint32_t");
  if (!STy) {
    std::cerr << "unable to find type 'uint32_t'!" << std::endl;
    return 1;
  }

  return 0;
}
