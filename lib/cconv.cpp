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

#include <dffi/dffi.h>
#include <clang/Basic/Specifiers.h>

// Static asserts that DFFI's calling convention enum has the same values as
// the clang one!

static_assert(dffi::CC_C == clang::CC_C, "inconsistency between dffi and clang calling conventions enum!")
static_assert(dffi::CC_X86StdCall == clang::CC_X86StdCall, "inconsistency between dffi and clang calling conventions enum!")
static_assert(dffi::CC_X86FastCall == clang::CC_X86FastCall, "inconsistency between dffi and clang calling conventions enum!")
static_assert(dffi::CC_X86ThisCall == clang::CC_X86ThisCall, "inconsistency between dffi and clang calling conventions enum!")
static_assert(dffi::CC_X86VectorCall == clang::CC_X86VectorCall, "inconsistency between dffi and clang calling conventions enum!")
static_assert(dffi::CC_X86Pascal == clang::CC_X86Pascal, "inconsistency between dffi and clang calling conventions enum!")
static_assert(dffi::CC_Win64 == clang::CC_Win64, "inconsistency between dffi and clang calling conventions enum!")
static_assert(dffi::CC_X86_64SysV == clang::CC_X86_64SysV, "inconsistency between dffi and clang calling conventions enum!")
static_assert(dffi::CC_X86RegCall == clang::CC_X86RegCall, "inconsistency between dffi and clang calling conventions enum!")
static_assert(dffi::CC_AAPCS == clang::CC_AAPCS, "inconsistency between dffi and clang calling conventions enum!")
static_assert(dffi::CC_AAPCS_VFP == clang::CC_AAPCS_VFP, "inconsistency between dffi and clang calling conventions enum!")
static_assert(dffi::CC_IntelOclBicc == clang::CC_IntelOclBicc, "inconsistency between dffi and clang calling conventions enum!")
static_assert(dffi::CC_SpirFunction == clang::CC_SpirFunction, "inconsistency between dffi and clang calling conventions enum!")
static_assert(dffi::CC_OpenCLKernel == clang::CC_OpenCLKernel, "inconsistency between dffi and clang calling conventions enum!")
static_assert(dffi::CC_Swift == clang::CC_Swift, "inconsistency between dffi and clang calling conventions enum!")
static_assert(dffi::CC_PreserveMost == clang::CC_PreserveMost, "inconsistency between dffi and clang calling conventions enum!")
static_assert(dffi::CC_PreserveAll == clang::CC_PreserveAll, "inconsistency between dffi and clang calling conventions enum!")
