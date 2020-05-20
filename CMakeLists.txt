cmake_minimum_required (VERSION 3.5.0)
project (planter)

set (CMAKE_INCLUDE_CURRENT_DIR ON)
set (CMAKE_CXX_STANDARD 17)
set (THREADS_PREFER_PTHREAD_FLAG ON)
set (CMAKE_THREAD_PREFER_PTHREAD TRUE)
set (CMAKE_CXX_FLAGS "-fcompare-debug-second")

find_package (Threads REQUIRED)

FILE (GLOB SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
FILE (GLOB HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

include_directories (BEFORE "${CMAKE_CURRENT_SOURCE_DIR}/")

SET (CMAKE_INCLUDE_CURRENT_DIR ON)

add_executable (${PROJECT_NAME} ${SOURCES} ${HEADERS})
target_link_libraries(${PROJECT_NAME} Threads::Threads -loniondebug -lonioni2c -lonionrelayexp -lmosquittopp)

