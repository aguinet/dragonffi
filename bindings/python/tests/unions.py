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
#

import pydffi
import struct
import sys

D=pydffi.FFI()
CU = D.compile('''
union A
{
  short a;
  int b;
};

short get_short(union A a) { return a.a; }
int get_int(union A a) { return a.b; }
''')

a = CU.types.A()
a.a = 10
assert(CU.funcs.get_short(a) == 10)

a = CU.types.A("b", 12345678)
assert(CU.funcs.get_int(a) == 12345678)
