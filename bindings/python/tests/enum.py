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
enum A {
    V0 = 0,
    V10 = 10
};

struct S {
    enum A a;
};

int get(enum A a) { return a; }
enum A get_a(struct S s) { return s.a; }
''')

A = CU.types.A
assert(int(A.V0) == 0)
assert(int(A.V10) == 10)

# CHECK-DAG: V0 = 0
# CHECK-DAG: V10 = 10
for k,v in iter(A):
    print("%s = %d" % (k,v))

# CHECK-DAG: V0 = 0
# CHECK-DAG: V10 = 10
for k,v in dict(A).items():
    print("%s = %d" % (k,v))

S = CU.types.S()
S.a = 10
A = S.a
assert(A == 10)
