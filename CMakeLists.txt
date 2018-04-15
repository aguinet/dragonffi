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
exec_program(${LLVM_CONFIG} ARGS --cmakedir OUTPUT_VARIABLE LLVM_CMAKE)
exec_program(${LLVM_CONFIG} ARGS --build-mode OUTPUT_VARIABLE LLVM_BUILD_MODE)
message(STATUS "LLVM build mode: ${LLVM_BUILD_MODE}")
if (${LLVM_BUILD_MODE} STREQUAL "Debug")
  set(LLVM_DEBUG TRUE)
endif()
string(REPLACE "\\" "/" LLVM_CMAKE "${LLVM_CMAKE}")
string(REPLACE "\\" "/" LLVM_BINDIR "${LLVM_BINDIR}")

set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" "${LLVM_CMAKE}")
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
include_directories(${CMAKE_CURRENT_BINARY_DIR}/include)
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
  DebugInfoDWARF
)
if (APPLE)
  set(LLVM_LINK_COMPONENTS ${LLVM_LINK_COMPONENTS} Demangle)
endif() 

set(CLANG_RES_DIR "${LLVM_BINDIR}/../lib${LLVM_LIBDIR_SUFFIX}/clang/${LLVM_VERSION}/")
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

llvm_map_components_to_libnames(llvm_libs ${LLVM_LINK_COMPONENTS})

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
  lib/dffi_api.cpp
  lib/dffi_impl.cpp
  lib/dffi_impl_clang.cpp
  lib/dffi_impl_clang_res.cpp
  lib/dffi_impl_dwarf.cpp
  lib/dffi_types.cpp
  lib/dffictx.cpp
)

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

option(DFFI_STATIC_LLVM "Create a static library with dffi and llvm/clang" OFF)

if (DFFI_STATIC_LLVM)
  add_definitions(-Ddffi_STATIC)
  add_library(dffi_static
    STATIC
    ${DFFI_SRC}
  )

  foreach(LIB_ ${DFFI_LINK_LIBRARIES})
    list(APPEND DFFI_LINK_LIBRARIES_FILES $<TARGET_FILE:${LIB_}>) 
  endforeach(LIB_)

  set(DFFI_STATIC_LLVM_PATH "${CMAKE_CURRENT_BINARY_DIR}/dffi_static_llvm${CMAKE_STATIC_LIBRARY_SUFFIX}" CACHE STRING "Path to the static library containings dffi and LLVM static libraries")
  if(WIN32)
    add_custom_command(
      TARGET dffi_static POST_BUILD
      COMMAND "LIB.EXE"
        ARGS
          "/OUT:${DFFI_STATIC_LLVM_PATH}"
          $<TARGET_FILE:dffi_static>
          ${DFFI_LINK_LIBRARIES_FILES}
    )
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
else()
  add_library(dffi
    SHARED
    ${DFFI_SRC}
  )
  target_link_libraries(dffi
    PUBLIC
    ${DFFI_LINK_LIBRARIES}
  )
endif()

add_custom_target(check)

option(PYTHON_BINDINGS "Compile python bindings with cmake" ON)
if (PYTHON_BINDINGS)
  add_subdirectory(bindings)
endif()
option(BUILD_TESTS "Build tests" ON)
add_subdirectory(tests)
