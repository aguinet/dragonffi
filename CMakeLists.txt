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

cmake_minimum_required(VERSION 3.10)
project(dragonffi)

find_program(LLVM_CONFIG NAMES llvm-config DOC "Path to llvm-config utility")
if (${LLVM_CONFIG} STREQUAL "LLVM_CONFIG-NOTFOUND")
  message(FATAL_ERROR "llvm-config not found, please manually set path with -DLLVM_CONFIG")
endif()
message(STATUS "Using llvm-config: ${LLVM_CONFIG}")
exec_program(${LLVM_CONFIG} ARGS --prefix OUTPUT_VARIABLE LLVM_PREFIX RETURN_VALUE LLVM_CONFIG_RET)
if (NOT ${LLVM_CONFIG_RET} EQUAL 0)
  message(FATAL_ERROR "Error while running llvm-config: ${LLVM_PREFIX}")
endif()
exec_program(${LLVM_CONFIG} ARGS --bindir OUTPUT_VARIABLE LLVM_BINDIR)
exec_program(${LLVM_CONFIG} ARGS --cmakedir OUTPUT_VARIABLE LLVM_CMAKE)
exec_program(${LLVM_CONFIG} ARGS --build-mode OUTPUT_VARIABLE LLVM_BUILD_MODE)
exec_program(${LLVM_CONFIG} ARGS --libs OUTPUT_VARIABLE LLVM_CONFIG_LIBFILES)
if (${LLVM_BUILD_MODE} STREQUAL "debug")
  set(LLVM_BUILD_DEBUG TRUE)
endif()
string(REPLACE "\\" "/" LLVM_PREFIX "${LLVM_PREFIX}")
string(REPLACE "\\" "/" LLVM_CMAKE "${LLVM_CMAKE}")
string(REPLACE "\\" "/" LLVM_BINDIR "${LLVM_BINDIR}")

message(STATUS "LLVM prefix: ${LLVM_PREFIX}")
message(STATUS "LLVM cmake: ${LLVM_CMAKE}")

set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(Clang_DIR "${LLVM_PREFIX}/lib/cmake/clang")
message(STATUS "Clang cmake directory: ${Clang_DIR}")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" "${LLVM_CMAKE}" "${Clang_DIR}")
set(LLVM_DIR "${LLVM_CMAKE}")
find_package(LLVM 11.1 REQUIRED)

# Clang
include(ClangConfig)
include_directories(${CLANG_INCLUDE_DIRS})
link_directories(${CLANG_LIBRARY_DIRS})
message(STATUS "Clang link dylib: ${CLANG_LINK_CLANG_DYLIB}")

# LLVM
include(AddLLVM)
include(LLVMConfig)

include_directories(${LLVM_INCLUDE_DIRS})
link_directories(${LLVM_LIBRARY_DIRS})

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/third-party)

set(LLVM_LINK_COMPONENTS
  ${LLVM_NATIVE_ARCH}
  AggressiveInstCombine
  Analysis
  AsmParser
  AsmPrinter
  BinaryFormat
  BitReader
  BitstreamReader
  BitWriter
  CFGuard
  CodeGen
  Core
  Coroutines
  Coverage
  DebugInfoCodeView
  DebugInfoMSF
  DebugInfoDWARF
  Demangle
  ExecutionEngine
  FrontendOpenMP
  GlobalISel
  IRReader
  InstCombine
  Instrumentation
  LTO
  Linker
  MC
  MCDisassembler
  MCJIT
  MCParser
  ObjCARCOpts
  ObjCARCOpts
  Object
  Option
  Passes
  ProfileData
  Remarks
  RuntimeDyld
  ScalarOpts
  SelectionDAG
  Support
  Target
  TextAPI
  TransformUtils
  Vectorize
  ipo
)
if (APPLE)
  set(LLVM_LINK_COMPONENTS ${LLVM_LINK_COMPONENTS} Demangle)
endif() 
llvm_map_components_to_libnames(llvm_libs ${LLVM_LINK_COMPONENTS})

set(CLANG_RES_DIR "${LLVM_BINDIR}/../lib${LLVM_LIBDIR_SUFFIX}/clang/${LLVM_VERSION}/include")
get_filename_component(CLANG_RES_DIR "${CLANG_RES_DIR}" ABSOLUTE)
message(STATUS "Clang resources directory: ${CLANG_RES_DIR}")

# Parse CLANG_RES_DIR and create a .cpp file with all its content
# This will be mapped into a virtual file system in dffi!
set(CLANG_RES_HEADER "${CMAKE_CURRENT_BINARY_DIR}/include/dffi/clang_res.h")

file(GLOB_RECURSE CLANG_RES_GLOB LIST_DIRECTORIES false "${CLANG_RES_DIR}/*")
add_custom_command(
  OUTPUT "${CLANG_RES_HEADER}"
  COMMAND "${CMAKE_COMMAND}" -DCLANG_RES_DIR="${CLANG_RES_DIR}" -DCLANG_RES_HEADER="${CLANG_RES_HEADER}" -P "${CMAKE_CURRENT_SOURCE_DIR}/CMakeClangRes.txt"
  DEPENDS ${CLANG_RES_GLOB} "${CMAKE_CURRENT_SOURCE_DIR}/CMakeClangRes.txt"
  COMMENT "Packing clang ressources into a header file...")


include(CheckTypeSize)

if (APPLE)
  # It seems to have an issue ith the libc++ on OSX: typeinfo(__int128_t) does not exist!
  # Disable it on this platform until we figure it out..
  set(HAVE_INT128_T 0)
else()
  SET(CMAKE_EXTRA_INCLUDE_FILES stdint.h)
  check_type_size("__int128_t" INT128_T)
  SET(CMAKE_EXTRA_INCLUDE_FILES)
endif()
check_type_size("_Complex" COMPLEX_T)

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/include/dffi/config.h.in" "${CMAKE_CURRENT_BINARY_DIR}/include/dffi/config.h" @ONLY)

set(DFFI_SRC
  lib/cconv.cpp
  lib/dffi_api.cpp
  lib/dffi_llvm_wrapper.cpp
  lib/dffi_impl.cpp
  lib/dffi_impl_clang.cpp
  lib/dffi_impl_clang_res.cpp
  lib/dffi_types.cpp
  lib/dffictx.cpp
  lib/types_printer.cpp
  lib/anon_member_inliner.cpp
)

get_source_file_property(_obj_depends lib/dffi_impl_clang_res.cpp OBJECT_DEPENDS)
if(NOT _obj_depends)
  set(_obj_depends)
endif()
list(APPEND _obj_depends "${CLANG_RES_HEADER}")
set_source_files_properties(lib/dffi_impl_clang_res.cpp PROPERTIES
  OBJECT_DEPENDS ${_obj_depends})

set(CLANG_LINK_LIBRARIES
  clangAST
  clangASTMatchers
  clangAnalysis
  clangBasic
  clangCodeGen
  clangDriver
  clangEdit
  clangFrontend
  clangFrontendTool
  clangLex
  clangParse
  clangRewrite
  clangRewriteFrontend
  clangSema
  clangSerialization
  clangCodeGen
)
set(DFFI_LINK_LIBRARIES
  ${llvm_libs}
)

option(DFFI_STATIC_LLVM "Create a static library with dffi and llvm/clang" OFF)

# From LLVM's DetermineGCCCompatible.cmake
if(NOT DEFINED DFFI_COMPILER_IS_GCC_COMPATIBLE)
  if(CMAKE_COMPILER_IS_GNUCXX)
    set(DFFI_COMPILER_IS_GCC_COMPATIBLE ON)
  elseif( MSVC )
    set(DFFI_COMPILER_IS_GCC_COMPATIBLE OFF)
  elseif( "${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang" )
    set(DFFI_COMPILER_IS_GCC_COMPATIBLE ON)
  elseif( "${CMAKE_CXX_COMPILER_ID}" MATCHES "Intel" )
    set(DFFI_COMPILER_IS_GCC_COMPATIBLE ON)
  endif()
endif()

if (NOT LLVM_ENABLE_RTTI)
  if (DFFI_COMPILER_IS_GCC_COMPATIBLE)
    set(DFFI_RTTI_FLAG -fno-rtti)
  elseif (MSVC)
    set(DFFI_RTTI_FLAG "/GR-")
  endif()
  # These files use Clang and/or LLVM structures where there typeinfo could
  # be used in DFFI's objects.
  # As we still need libdffi to be compiled with RTTI (because pybind11
  # requires it), we only explicitly disable RTTI for these C++ files.
  set_source_files_properties(
    lib/dffi_impl_clang.cpp
    lib/dffi_llvm_wrapper.cpp
    PROPERTIES
    COMPILE_FLAGS ${DFFI_RTTI_FLAG})
endif()

# Backported from LLVM master
function(clang_target_link_libraries target type)
  if (CLANG_LINK_CLANG_DYLIB)
    target_link_libraries(${target} ${type} clang-cpp)
  else()
    target_link_libraries(${target} ${type} ${ARGN})
  endif()
endfunction()

if (DFFI_STATIC_LLVM)
  add_definitions(-Ddffi_STATIC)
  add_library(dffi_static
    STATIC
    ${DFFI_SRC}
  )

  foreach(LIB_ ${DFFI_LINK_LIBRARIES} ${CLANG_LINK_LIBRARIES})
    list(APPEND DFFI_LINK_LIBRARIES_FILES $<TARGET_FILE:${LIB_}>) 
  endforeach(LIB_)

  set(DFFI_STATIC_LLVM_PATH "${CMAKE_CURRENT_BINARY_DIR}/dffi_static_llvm${CMAKE_STATIC_LIBRARY_SUFFIX}" CACHE STRING "Path to the static library containings dffi and LLVM static libraries")
  if(WIN32)
    # We put arguments in a separate files, otherwise it might become too big
    # for cmd.exe to handle.
    set(LIB_COMMAND_ARGS_FILE "${CMAKE_CURRENT_BINARY_DIR}/lib_exe_args")
    string(JOIN " " LINK_LIBS ${DFFI_LINK_LIBRARIES_FILES})
    file(GENERATE OUTPUT ${LIB_COMMAND_ARGS_FILE} CONTENT
      "/OUT:${DFFI_STATIC_LLVM_PATH} $<TARGET_FILE:dffi_static> ${LINK_LIBS}")

    add_custom_command(
      TARGET dffi_static POST_BUILD
      COMMAND "LIB.EXE"
        ARGS "@${LIB_COMMAND_ARGS_FILE}")
  else()
    if (APPLE)
      set(MERGE_LIBS "${CMAKE_CURRENT_SOURCE_DIR}/tools/merge_libs_osx.sh")
    elseif(UNIX)
      set(MERGE_LIBS "${CMAKE_CURRENT_SOURCE_DIR}/tools/merge_libs_gnu.sh")
    else()
      message(FATAL_ERROR "Static llvm library: unsupported system ${CMAKE_SYSTEM_NAME}")
    endif()
    add_custom_command(
      TARGET dffi_static POST_BUILD
      COMMAND "${MERGE_LIBS}"
        ARGS
          "${DFFI_STATIC_LLVM_PATH}"
          $<TARGET_FILE:dffi_static>
          ${DFFI_LINK_LIBRARIES_FILES}
      DEPENDS "${MERGE_LIBS}"
    )
  endif()
  set(DFFI_LIBRARY dffi_static)
else()
  add_library(dffi
    SHARED
    ${DFFI_SRC}
  )
  if (LLVM_LINK_LLVM_DYLIB)
    llvm_config(dffi
      USE_SHARED
      ${LLVM_LINK_COMPONENTS}
    )
  else()
    llvm_config(dffi
      ${LLVM_LINK_COMPONENTS}
    )
  endif()
  if (UNIX)
    target_link_libraries(dffi PRIVATE dl)
  endif()
  clang_target_link_libraries(dffi PRIVATE ${CLANG_LINK_LIBRARIES})
  set(DFFI_LIBRARY dffi)
endif()

add_custom_target(check)
add_subdirectory(bindings)

option(BUILD_TESTS "Build tests" ON)
add_subdirectory(tests)
add_subdirectory(examples)
