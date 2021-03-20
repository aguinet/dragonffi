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

# Configure lit
configure_file("lit.site.cfg.in" "${CMAKE_CURRENT_BINARY_DIR}/lit.site.cfg" ESCAPE_QUOTES @ONLY)

if (BUILD_TESTS)
  set(TESTS
    anon_members
    anon_struct
    anon_union
    array
    asm_redirect
    attrs
    bool
    cconv
    compile
    compile_error
    decl
    dlopen
    enum
    func_ptr
    includes
    inline
    lasterror
    multiple_defs
    stdint
    struct
    system_headers
    typedef
    union
    varargs
  )

  # Compile tests
  foreach(TEST ${TESTS})
    add_executable(${TEST} ${TEST}.cpp)
    target_link_libraries(${TEST} dffi)
  endforeach()

  find_package(PythonInterp REQUIRED)
  add_custom_target(check_lib
      COMMAND "${PYTHON_EXECUTABLE}" -m lit.main "${CMAKE_CURRENT_BINARY_DIR}" -v)
  add_dependencies(check_lib ${TESTS})
  add_dependencies(check check_lib) 
  set(PYTHON_EXECUTABLE ${PYTHON_EXECUTABLE_SAVE})
endif()
