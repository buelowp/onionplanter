cmake_minimum_required (VERSION 3.5.0)
project (planter)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

FILE (GLOB SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
FILE (GLOB HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

include_directories (BEFORE "${CMAKE_CURRENT_SOURCE_DIR}/")

SET (CMAKE_INCLUDE_CURRENT_DIR ON)

add_executable (${PROJECT_NAME} ${SOURCES} ${HEADERS})
target_link_libraries(${PROJECT_NAME} -loniondebug -lonioni2c -lonionoledexp -lonionmcp23008 -lonionrelayexp)

