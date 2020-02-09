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

import unittest
import pydffi

from common import DFFITest

class FuncPtrTest(DFFITest):
    def test_func_ptr(self):
        FFI = self.FFI
        CU = FFI.compile('''
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

        Add=CU.funcs.get_op(0)
        Add=Add.obj
        Res=Add(1,4)
        self.assertEqual(Res.res, 5)

        Res=CU.funcs.call(pydffi.ptr(CU.funcs.sub), 1, 5)
        self.assertEqual(Res.res, -4)

if __name__ == '__main__':
    unittest.main()
