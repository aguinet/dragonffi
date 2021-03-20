# coding: utf-8
# Copyright 2021 Adrien Guinet <adrien@guinet.me>
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
import platform

from common import DFFITest

class LastErrorTest(DFFITest):
    def test_lasterror(self):
        J=self.FFI

        if platform.system() == 'Windows':
            code = '''
#include <windows.h>
void seterrno(int val) {
  SetLastError(val);
}
'''
        else:
            code = '''
#include <errno.h>
void seterrno(int val) {
  errno = val;
}
'''
        CU = J.compile(code, useLastError=True)
        CU.funcs.seterrno(10)
        self.assertEqual(pydffi.getLastError(), 10)

if __name__ == '__main__':
    unittest.main()
