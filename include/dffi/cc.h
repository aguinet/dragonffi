// Copyright 2018 Adrien Guinet <adrien@guinet.me>
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef DFFI_CC_H
#define DFFI_CC_H

namespace dffi {

// This enum is mapped on clang's calling convention values (see
// clang/Basic/Specifiers.h). Static asserts are made to ensure the
// consistency!
enum CallingConv {
  CC_C,           // __attribute__((cdecl))
  CC_X86StdCall,  // __attribute__((stdcall))
  CC_X86FastCall, // __attribute__((fastcall))
  CC_X86ThisCall, // __attribute__((thiscall))
  CC_X86VectorCall, // __attribute__((vectorcall))
  CC_X86Pascal,   // __attribute__((pascal))
  CC_Win64,       // __attribute__((ms_abi))
  CC_X86_64SysV,  // __attribute__((sysv_abi))
  CC_X86RegCall, // __attribute__((regcall))
  CC_AAPCS,       // __attribute__((pcs("aapcs")))
  CC_AAPCS_VFP,   // __attribute__((pcs("aapcs-vfp")))
  CC_IntelOclBicc, // __attribute__((intel_ocl_bicc))
  CC_SpirFunction, // default for OpenCL functions on SPIR target
  CC_OpenCLKernel, // inferred for OpenCL kernels
  CC_Swift,        // __attribute__((swiftcall))
  CC_PreserveMost, // __attribute__((preserve_most))
  CC_PreserveAll,  // __attribute__((preserve_all))
};

const char* CCToClangAttribute(CallingConv CC);

} // dffi

#endif
