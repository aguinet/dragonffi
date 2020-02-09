# coding: utf-8
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

class StrTest(DFFITest):
    def test_str(self):
        J=self.FFI

        CU = J.compile('''
#include <stdio.h>
#include <stdbool.h>

bool check_str0(const char* msg) {
    return strcmp(msg, "coucou") == 0;
}
bool check_str1(const char* msg) {
    return strcmp(msg, "héllo") == 0;
}
const char* get_str() { return "hello"; }
        ''')
        get_str = CU.funcs.get_str

        self.assertTrue(CU.funcs.check_str0("coucou"))
        self.assertTrue(CU.funcs.check_str1("héllo"))

        self.assertEqual((get_str().cstr).tobytes(), b"hello")

if __name__ == '__main__':
    unittest.main()
