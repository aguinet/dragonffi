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
struct A {
    int a;
    short b;
};
void print(struct A a) {
    printf("%d %d\\n", a.a, a.b);
}
''')

a = CU.types.A(a=4,b=10)
# CHECK: 4 10
getattr(CU.funcs, "print")(a)

a = CU.types.A(a=0,b=0)
# CHECK: 0 0
getattr(CU.funcs, "print")(a)
