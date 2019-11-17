INCLUDE( FindPackageHandleStandardArgs )

IF( "${LIBCLANG_DIR}" STREQUAL "" )
  set(llvm_config_names llvm-config)
  foreach(major RANGE 9 3)
    list(APPEND llvm_config_names "llvm-config${major}" "llvm-config-${major}")
    foreach(minor RANGE 9 0)
      list(APPEND llvm_config_names "llvm-config${major}${minor}" "llvm-config-${major}.${minor}" "llvm-config-mp-${major}.${minor}")
    endforeach ()
  endforeach ()
  find_program(LIBCLANG_LLVM_CONFIG_EXECUTABLE NAMES ${llvm_config_names})
  execute_process(COMMAND ${LIBCLANG_LLVM_CONFIG_EXECUTABLE} --prefix OUTPUT_VARIABLE LIBCLANG_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)
ENDIF()

FIND_PATH( LIBCLANG_INCLUDE_DIR
  clang-c/Index.h
  PATH_SUFFIXES include
  HINTS ${LIBCLANG_DIR}
)

FIND_LIBRARY( LIBCLANG_LIBRARY
  clang
  PATH_SUFFIXES lib
  HINTS ${LIBCLANG_DIR}
)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(
  LIBCLANG
  DEFAULT_MSG
  LIBCLANG_LIBRARY
  LIBCLANG_INCLUDE_DIR
)

IF( LIBCLANG_FOUND )
  SET( LIBCLANG_LIBRARIES    ${LIBCLANG_LIBRARY} )
  SET( LIBCLANG_INCLUDE_DIRS ${LIBCLANG_INCLUDE_DIR} )

  MARK_AS_ADVANCED(
    LIBCLANG_LIBRARY
    LIBCLANG_INCLUDE_DIR
  )
ENDIF()
