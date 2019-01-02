option(PYTHON_BINDINGS "Compile python bindings with cmake" ON)
if (PYTHON_BINDINGS)
  add_subdirectory(python)
endif()
