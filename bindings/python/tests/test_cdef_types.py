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

import unittest
import pydffi

from common import DFFITest

class CDefTypesTest(DFFITest):
    def test_cdef_types(self):
        CU = self.FFI.cdef('''
#include <stdint.h>
typedef int32_t MyInt;
typedef struct {
  int a;
  int b;
} A;
        ''')

        self.assertEqual(CU.types.MyInt, self.FFI.Int32Ty)
        self.assertTrue(isinstance(CU.types.A, pydffi.StructType))


if __name__ == '__main__':
    unittest.main()
