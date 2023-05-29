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

class AnonStructTest(DFFITest): 
    def test_anon_struct(self):
        CU = self.FFI.compile('''
        #include <stdio.h>
        struct A {
          struct {
            int a;
            int b;
          } s;
        };

        void dump(struct A a) {
          printf("s.a=%d, s.b=%d\\n", a.s.a, a.s.b);
        }
        ''')

        A = CU.types.A()
        Obj = A.s
        Ty = pydffi.typeof(Obj)
        fields = [f.name for f in iter(Ty)]
        self.assertEqual(set(fields), set(("a","b")))

if __name__ == '__main__':
    unittest.main()
