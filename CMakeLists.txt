CMAKE_MINIMUM_REQUIRED( VERSION 2.9 )
PROJECT( codesim CXX )

SET( CMAKE_EXPORT_COMPILE_COMMANDS ON )

LIST( APPEND CMAKE_MODULE_PATH
  ${CMAKE_SOURCE_DIR}/cmake/Modules
)

SET( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall")

FIND_PACKAGE( libclang REQUIRED )
INCLUDE_DIRECTORIES(${LIBCLANG_INCLUDE_DIRS} SYSTEM)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/capted/lib)

ADD_EXECUTABLE(codesim codesim.cpp)
TARGET_LINK_LIBRARIES(codesim ${LIBCLANG_LIBRARIES})
