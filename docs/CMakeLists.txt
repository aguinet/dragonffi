# TODO: do not build the python bindings documentation if not compiled?
if (NOT PYTHON_BINDINGS)
  message(FATAL_ERROR "Python bindings must be built to generate the documentation.")
endif()

add_subdirectory(doxygen)

include(Python)
find_package(Python3 REQUIRED)
find_python_module(sphinx_rtd_theme)
find_program(SPHINX_BUILD_BIN NAMES sphinx-build)
message(STATUS "Sphinx build binary: ${SPHINX_BUILD_BIN}")

if(NOT SPHINX_BUILD_BIN)
  message(FATAL_ERROR "Can't build doc: sphinx-build not found")
endif()
if(NOT PY_SPHINX_RTD_THEME)
  message(FATAL_ERROR "Can't build doc: sphinx_rtd_theme python package not found")
endif()

set(DFFI_DOC_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/_build")
set(SPHINX_CACHE_DIR "${CMAKE_CURRENT_BINARY_DIR}/_doctrees")
set(DFFI_DOC_OUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/dffi-doc")

file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/pydffi_include.py"
  CONTENT "import sys,os\nsys.path.insert(0, os.path.abspath(\"$<TARGET_FILE_DIR:pydffi>\"))\n")

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/conf.py.in"
  "${DFFI_DOC_BUILD_DIR}/conf.py"
  @ONLY)

add_custom_target(doc ALL
  COMMAND
    "${SPHINX_BUILD_BIN}"
    -c "${DFFI_DOC_BUILD_DIR}"
    -d "${SPHINX_CACHE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${DFFI_DOC_OUT_DIR}"
  COMMENT "Building documentation with Sphinx..."
  # TODO DEPENDS
)

add_dependencies(doc doc_doxygen pydffi)
