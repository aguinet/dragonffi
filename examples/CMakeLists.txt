if (BUILD_TESTS)
  set(EXAMPLES_DIR "${PROJECT_SOURCE_DIR}/examples")
  if (PYTHON_BINDINGS)
    set(EXAMPLES
      "${EXAMPLES_DIR}/archive.py"
      "${EXAMPLES_DIR}/fftw.py"
      "${EXAMPLES_DIR}/readdir.py"
    )

    set(PYDFFI_LIB_DIR $<TARGET_FILE_DIR:pydffi>)
    configure_file("lit.site.cfg.in" "${CMAKE_CURRENT_BINARY_DIR}/lit.site.cfg" ESCAPE_QUOTES @ONLY)
    add_custom_target(check_python_examples
      COMMAND ${CMAKE_COMMAND} -E env PYDFFI_DIR=${PYDFFI_LIB_DIR} "${PYTHON_EXECUTABLE}" "${LIT_RUNNER}" "${CMAKE_CURRENT_BINARY_DIR}" -v
      DEPENDS ${EXAMPLES}
    )
    add_dependencies(check check_python_examples)
  endif()

endif()
