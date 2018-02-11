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

#ifndef DFFI_EXPORTS_H
#define DFFI_EXPORTS_H

#if defined _WIN32 || defined __CYGWIN__
  #define DFFI_HELPER_IMPORT __declspec(dllimport)
  #define DFFI_HELPER_EXPORT __declspec(dllexport)
  #define DFFI_HELPER_LOCAL
#else
  #define DFFI_HELPER_IMPORT __attribute__ ((visibility ("default")))
  #define DFFI_HELPER_EXPORT __attribute__ ((visibility ("default")))
  #define DFFI_HELPER_LOCAL  __attribute__ ((visibility ("hidden")))
#endif

#ifdef dffi_STATIC
  #define DFFI_API
  #define DFFI_LOCAL
#elif defined(dffi_EXPORTS)
  #define DFFI_API DFFI_HELPER_EXPORT
  #define DFFI_LOCAL DFFI_HELPER_LOCAL
#else
  #define DFFI_API DFFI_HELPER_IMPORT
  #define DFFI_LOCAL DFFI_HELPER_LOCAL
#endif

#endif
