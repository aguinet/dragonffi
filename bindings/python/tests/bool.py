# Copyright 2018 Adrien Guinet <adrien@guinet.me>
# 
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
#     http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# RUN: "%python" "%s"
# coding: utf-8

import pydffi
import random

J=pydffi.FFI()

CU = J.compile('''
#include <stdbool.h>
bool foo(int a) { return a==1; }
bool invert(const bool v) { return !v; }
''')

assert(CU.funcs.foo(1) == True)
assert(CU.funcs.foo(0) == False)
assert(CU.funcs.invert(True) == False)
assert(CU.funcs.invert(False) == True)

b = CU.funcs.foo(1)
assert(b == True)
assert((not b) == False)
assert((b & False) == False)
assert((b & True) == True)
