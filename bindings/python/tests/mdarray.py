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
import struct

N = 10
FFI = pydffi.FFI()
ArTy = FFI.arrayType(FFI.DoubleTy, N)
Ar = ArTy()
for i in range(len(Ar)):
    Ar[i] = i
Ref = [float(i) for i in range(len(Ar))]
assert(list(Ar) == Ref)
ArV = memoryview(Ar)
assert(ArV.format == "d")
def upck_double(v):
    if isinstance(v,float):
        return v
    return struct.unpack("d",v)[0]
assert(all(upck_double(a) == b for a,b in zip(ArV,Ref)))

Ar2Ty = FFI.arrayType(FFI.arrayType(FFI.DoubleTy, 10), 2)
Ar = Ar2Ty()
ArV = memoryview(Ar)
assert(ArV.format == "d")
assert(ArV.ndim == 2)
assert(ArV.shape == (2,10))
assert(ArV.strides == (80,8))

try:
    import numpy as np
except ImportError:
    sys.exit(0)

ar = np.ndarray(N)
car = pydffi.view_as(ArTy, ar)
assert(all(a == b for a,b in zip(ar,car)))
