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

J=pydffi.FFI()
CU = J.compile('''
typedef struct
{
  int a;
  int b;
  int res;
} Res;

typedef Res(*op)(int,int);

static Res get_res(int res, int a, int b) {
  Res Ret = {a,b,res};
  return Ret;
}

static Res add(int a, int b) {
  return get_res(a+b,a,b);
}
static Res sub(int a, int b) {
  return get_res(a-b,a,b);
}

op get_op(unsigned Id)
{
  if (Id == 0) return add;
  if (Id == 1) return sub;
  return 0;
}

Res call(op f, int a, int b) {
  return f(a,b);
}
''')

Add=CU.getFunction("get_op").call(0)
Add=Add.obj
Res=Add.call(1,4)
assert(Res.res == 5)

Res=CU.getFunction("call").call(J.ptr(CU.getFunction("sub")), 1, 5)
assert(Res.res == -4)
