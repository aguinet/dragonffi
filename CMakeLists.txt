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

cmake_minimum_required(VERSION 3.4.3)
project(dragonffi)

find_program(LLVM_CONFIG NAMES llvm-config DOC "Path to llvm-config utility")
if (${LLVM_CONFIG} STREQUAL "LLVM_CONFIG-NOTFOUND")
  message(FATAL_ERROR "llvm-config not found, please manually set path with -DLLVM_CONFIG")
endif()
message(STATUS "Using llvm-config: ${LLVM_CONFIG}")
exec_program(${LLVM_CONFIG} ARGS --prefix OUTPUT_VARIABLE LLVM_PREFIX)
exec_program(${LLVM_CONFIG} ARGS --bindir OUTPUT_VARIABLE LLVM_BINDIR)

set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" "${LLVM_PREFIX}/lib/cmake/llvm")
set(Clang_DIR "${LLVM_PREFIX}/lib/cmake/clang")

include(PrecompiledHeader)

# Clang
find_package(Clang)
include_directories(${CLANG_INCLUDE_DIRS})
link_directories(${CLANG_LIBRARY_DIRS})

# LLVM
include(HandleLLVMOptions)
include(AddLLVM)
include(LLVMConfig)

if (NOT ${LLVM_ENABLE_RTTI})
  message(FATAL_ERROR "LLVM must be compiled with -DLLVM_ENABLE_RTTI=ON!")
endif()

include_directories(${LLVM_INCLUDE_DIRS})
link_directories(${LLVM_LIBRARY_DIRS})

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/third-party)

set(LLVM_LINK_COMPONENTS
  ${LLVM_TARGETS_TO_BUILD}
  Analysis
  AsmParser
  AsmPrinter
  BinaryFormat
  BitReader
  BitWriter
  CodeGen
  Core
  Coroutines
  Coverage
  DebugInfoCodeView
  DebugInfoMSF
  ExecutionEngine
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
  RuntimeDyld
  ScalarOpts
  SelectionDAG
  Support
  Target
  TransformUtils
  Vectorize
  ipo
)

set(CLANG_RES_DIR "${LLVM_BINDIR}/../lib${LLVM_LIBDIR_SUFFIX}/clang/${LLVM_VERSION}/")
get_filename_component(CLANG_RES_DIR "${CLANG_RES_DIR}" ABSOLUTE)
message(STATUS "Clang resources directory: ${CLANG_RES_DIR}")

# Parse CLANG_RES_DIR and create a .cpp file with all its content
# This will be mapped into a virtual file system in dffi!
set(CLANG_RES_HEADER "${CMAKE_CURRENT_BINARY_DIR}/include/dffi/clang_res.h")
include_directories("${CMAKE_CURRENT_BINARY_DIR}/include")

file(GLOB_RECURSE CLANG_RES_GLOB LIST_DIRECTORIES false "${CLANG_RES_DIR}/*")
add_custom_command(
  OUTPUT "${CLANG_RES_HEADER}"
  COMMAND "${CMAKE_COMMAND}" -DCLANG_RES_DIR="${CLANG_RES_DIR}" -DCLANG_RES_HEADER="${CLANG_RES_HEADER}" -P "${CMAKE_CURRENT_SOURCE_DIR}/CMakeClangRes.txt"
  DEPENDS ${CLANG_RES_GLOB} "${CMAKE_CURRENT_SOURCE_DIR}/CMakeClangRes.txt"
  COMMENT "Packing clang ressources into a header file...")

llvm_map_components_to_libnames(llvm_libs ${LLVM_LINK_COMPONENTS})

add_library(dffi_objs
  OBJECT
  lib/dffi_api.cpp
  lib/dffi_impl.cpp
  lib/dffi_impl_clang.cpp
  lib/dffi_impl_clang_res.cpp
  lib/dffi_types.cpp
  lib/dffictx.cpp
)

add_library(dffi $<TARGET_OBJECTS:dffi_objs>)

get_source_file_property(_obj_depends lib/dffi_impl_clang_res.cpp OBJECT_DEPENDS)
if(NOT _obj_depends)
  set(_obj_depends)
endif()
list(APPEND _obj_depends "${CLANG_RES_HEADER}")
set_source_files_properties(lib/dffi_impl_clang_res.cpp PROPERTIES
  OBJECT_DEPENDS ${_obj_depends})



set(DFFI_LINK_LIBRARIES
  clangARCMigrate
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
  ${llvm_libs}
)

target_link_libraries(dffi
  PUBLIC
  ${DFFI_LINK_LIBRARIES}
)

option(DFFI_STATIC_LLVM "Create a static library with dffi and llvm/clang" OFF)

if (DFFI_STATIC_LLVM)
  add_library(dffi_static
    STATIC
    $<TARGET_OBJECTS:dffi_objs>
  )

  foreach(LIB_ ${DFFI_LINK_LIBRARIES})
    list(APPEND DFFI_LINK_LIBRARIES_FILES $<TARGET_FILE:${LIB_}>) 
  endforeach(LIB_)

  set(DFFI_STATIC_LLVM_PATH "${CMAKE_CURRENT_BINARY_DIR}/dffi_static_llvm${CMAKE_STATIC_LIBRARY_SUFFIX}" CACHE STRING "Path to the static library containings dffi and LLVM static libraries")
  # TODO: make merge libs for {dar,}win
  set(MERGE_LIBS "${CMAKE_CURRENT_SOURCE_DIR}/merge_libs.sh")
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

add_custom_target(check)

option(PYTHON_BINDINGS "Compile python bindings with cmake" ON)
if (PYTHON_BINDINGS)
  add_subdirectory(bindings)
endif()
option(BUILD_TESTS "Build tests" ON)
if (BUILD_TESTS)
  add_subdirectory(tests)
endif()
