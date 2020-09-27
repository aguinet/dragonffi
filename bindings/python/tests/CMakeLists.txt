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

set(PYDFFI_LIB_DIR $<TARGET_FILE_DIR:pydffi>)
set(DFFI_LIB_PATH $<TARGET_FILE:dffi>)
add_custom_target(check_python
  COMMAND ${CMAKE_COMMAND} -E env PYTHONPATH=${PYDFFI_LIB_DIR} "${PYTHON_EXECUTABLE}" -m unittest discover "${CMAKE_CURRENT_SOURCE_DIR}"
)
if (WIN32)
  add_custom_target(copy_dll
    COMMAND ${CMAKE_COMMAND} -E copy ${DFFI_LIB_PATH} ${PYDFFI_LIB_DIR}
  )
  add_dependencies(check_python copy_dll)
endif()
add_dependencies(check_python pydffi)
add_dependencies(check check_python)
