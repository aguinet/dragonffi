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
# Create everything and only returns a specific object. This object should
# still be usable!

import unittest
import pydffi

from common import getFFI

class MemRefsTest(unittest.TestCase):
    def get_CU(self):
        FFI = getFFI()
        return FFI.compile('''
        struct A {
          short a;
          int b;
        };

        short get_a(struct A a) { return a.a; };
        int get_b(struct A a) { return a.b; };
        ''')

    def run_CU(self):
        CU = self.get_CU()
        SA = CU.types.A
        A = pydffi.CStructObj(SA)
        A.b = 2
        self.assertEqual(CU.funcs.get_b(A).value, 2)

    def get_fun(self):
        return getFFI().compile('''
        unsigned fact(unsigned n) {
          unsigned ret = 1;
          for (unsigned i = 2; i <= n; ++i) {
              ret *= i;
          }
          return ret;
        }
        ''').getFunction("fact")

    def run_fun(self):
        fact = self.get_fun()
        def pyfact(n):
            n = 1
            for i in range(2,n+1): n *= i
            return n
        self.assertEqual(fact.call(6).value, pyfact(6))

    def get_A(self):
        FFI = getFFI()
        CU = FFI.cdef('''
        struct A {
          short a;
          int b;
        };
        // TODO: force declaration of struct A...
        short __foo(struct A a) { return a.a; }
        ''')

        SA = CU.types.A
        return pydffi.CStructObj(SA)

    def run_A(self):
        A = self.get_A()
        A.a = 1
        A.b = 5
        self.assertEqual(A.a, 1)
        self.assertEqual(A.b, 5)

    def get_buf_view(self):
        FFI = getFFI()
        buf = bytearray(b"hello\x00")
        return FFI,pydffi.view_as(FFI.arrayType(FFI.UInt8Ty, len(buf)), buf)

    def run_buf_view(self):
        FFI,buf = self.get_buf_view()
        CU = FFI.compile('''
        #include <stdint.h>
        #include <stdio.h>
        void print_(uint8_t* msg) {
            puts(msg);
        }
        ''')
        # CHECK: hello
        CU.funcs.print_(buf)

    def test_memrefs(self):
        self.run_CU()
        self.run_A()
        self.run_buf_view()

        self.assertEqual(getFFI().compile("int foo(int a, int b) { return a+b; }").funcs.foo(1,4), 5)

if __name__ == '__main__':
    unittest.main()
