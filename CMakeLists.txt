cmake_minimum_required(VERSION 2.8.12)

set(Boost_USE_STATIC_LIBS        ON)  # only find static libs
set(Boost_USE_DEBUG_LIBS         OFF) # ignore debug libs and
set(Boost_USE_RELEASE_LIBS       ON)  # only find release libs
set(Boost_USE_MULTITHREADED      ON)
set(Boost_USE_STATIC_RUNTIME    OFF)
find_package( Boost 1.67 COMPONENTS program_options date_time filesystem system REQUIRED )

include_directories(. include PicoSHA2 ${Boost_INCLUDE_DIR})

file(GLOB SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc")

set( CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -std=c++14" )

add_executable(httpsrv ${SOURCES})

target_link_libraries(httpsrv LINK_PUBLIC -pthread ${Boost_LIBRARIES})

