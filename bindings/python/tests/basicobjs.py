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
import random

D = pydffi.FFI()
for Ty in (D.Int8Ty, D.UInt8Ty, D.Int16Ty, D.UInt16Ty, D.Int32Ty, D.UInt32Ty, D.Int64Ty, D.UInt64Ty):
    for i in range(10):
        v = random.getrandbits(Ty.size*8-1)
        assert(Ty(v) == Ty(v))
        assert(Ty(v) == v)
        assert(Ty(0) <= Ty(v))
        assert(Ty(0) <= v)
        assert(Ty(v) >= Ty(0))
        assert(Ty(v) >= 0)
