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

# RUN: "%python" "%s" | "%FileCheck" "%s"
#

import pydffi

D = pydffi.FFI()
CU = D.compile('''
#include <stdio.h>
int foo() { return 42; }

struct A {
    int a;
};
__attribute__((ms_abi)) void print(struct A a) {
    printf("%d\\n", a.a);
}

void print_int(int a) { printf("%d\\n", a); }
''')
assert(CU.funcs.foo().value == 42)

a = CU.types.A(a=15)
# CHECK: 15
getattr(CU.funcs, "print")(a)

# CHECK: 42
CU.funcs.print_int(D.Int32(42))
# CHECK: 42
CU.funcs.print_int(42)
