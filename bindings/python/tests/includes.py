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

# RUN: "%python" "%s" "%S/../../../tests/includes"
#

import pydffi
import sys

F = pydffi.FFI(includeDirs=[sys.argv[1]])
CU = F.compile('''
#include "add.h"
''')
assert(CU.funcs.add(4,5) == 9)

try:
    F = pydffi.FFI()
    CU = F.compile('''
    #include "add.h"
    ''')
except pydffi.CompileError:
    sys.exit(0)

print("Last compilatin should have failed!")
sys.exit(1)
