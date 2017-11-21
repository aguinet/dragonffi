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

import pydffi
import sys

J=pydffi.FFI()
try:
    J.compile("this is invalid code")
    sys.exit(1)
except Exception as e:
    sys.exit(0)

try:
    J.compile("this is invalid code again")
    sys.exit(1)
except pydffi.CompileError:
    sys.exit(0)

try:
    J.compile("int foo(int a, int b) { return a+b; }")
    sys.exit(0)
except pydffi.CompileError:
    sys.exit(1)
