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

#ifndef DFFI_COMPAT_H
#define DFFI_COMPAT_H

// Adapted from LLVM's Compiler.h

#ifndef __has_cpp_attribute
#define __has_cpp_attribute(x) 0
#endif

#if __cplusplus > 201402L && __has_cpp_attribute(nodiscard)
#define DFFI_NODISCARD [[nodiscard]]
#elif !__cplusplus
// Workaround for llvm.org/PR23435, since clang 3.6 and below emit a spurious
// error when __has_cpp_attribute is given a scoped attribute in C mode.
#define DFFI_NODISCARD
#elif __has_cpp_attribute(clang::warn_unused_result)
#define DFFI_NODISCARD [[clang::warn_unused_result]]
#else
#define DFFI_NODISCARD
#endif

#ifdef _MSC_VER
#define ALIGN(N) __declspec(align(N))
#else
#define ALIGN(N) __attribute__((aligned(N)))
#endif

#endif
