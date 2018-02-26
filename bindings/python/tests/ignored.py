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
import pydffi
import sys

def check(CU, name):
    try:
        CU.getFunction(name)
        sys.exit(1)
    except pydffi.UnknownFunctionError: pass

D = pydffi.FFI()
CU = D.compile('''
#include <stdio.h>
#include <stdlib.h>
__attribute__((noreturn)) void fatal(const char* err) {
    puts(err);
    exit(1);
}
''')
check(CU, "fatal")

CU = D.cdef('''
__attribute__((noreturn)) void fatal(const char* err);
''')
check(CU, "fatal")
