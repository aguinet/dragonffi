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

import pydffi
import unittest

from common import DFFITest

try:
    long(0)
except:
    long = lambda v: int(v)

class EnumTest(DFFITest):
    def test_enum(self):
        D = self.FFI
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
        self.assertEqual(int(A.V0),  0)
        self.assertEqual(int(A.V10), 10)

        self.assertEqual(set(iter(A)), {(u"V0",long(0)),(u"V10",long(10))})
        self.assertEqual(set(dict(iter(A)).items()), {(u"V0",long(0)),(u"V10",long(10))})

        S = CU.types.S()
        S.a = 10
        A = S.a
        self.assertEqual(A, 10)

if __name__ == '__main__':
    unittest.main()
