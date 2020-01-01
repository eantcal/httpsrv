cmake_minimum_required(VERSION 2.8.12)

include_directories(. include PicoSHA2)

file(GLOB SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc")

set( CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -std=c++14" )

add_executable(httpsrv ${SOURCES})

target_link_libraries(httpsrv -pthread)
