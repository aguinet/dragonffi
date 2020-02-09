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
import os

from common import DFFITest

class IncludesTest(DFFITest):
    def test_includes(self):
        tests_dir = os.path.join(os.path.dirname(__file__), "..", "..", "..", "tests", "includes")
        F = pydffi.FFI(includeDirs=[tests_dir])
        CU = F.compile('''
#include "add.h"
        ''')
        self.assertEqual(CU.funcs.add(4,5), 9)

        with self.assertRaises(pydffi.CompileError):
            CU = self.FFI.compile('''
#include "add.h"
            ''')

if __name__ == '__main__':
    unittest.main()
