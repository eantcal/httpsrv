cmake_minimum_required(VERSION 2.8.12)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

#c++17 flag is required for shared_mutex
set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wno-missing-field-initializers -std=c++17")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O2")


set(Boost_USE_STATIC_LIBS        ON)  # only find static libs
set(Boost_USE_DEBUG_LIBS         OFF) # ignore debug libs and
set(Boost_USE_RELEASE_LIBS       ON)  # only find release libs
set(Boost_USE_MULTITHREADED      ON)
set(Boost_USE_STATIC_RUNTIME    OFF)
find_package( Boost 1.67 COMPONENTS filesystem REQUIRED )

include_directories(. include 3pp/PicoSHA2 3pp/zip/src ${Boost_INCLUDE_DIR})

file(GLOB SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc" "${CMAKE_CURRENT_SOURCE_DIR}/3pp/zip/src/*.c" )

add_executable(httpsrv ${SOURCES})

target_link_libraries(httpsrv LINK_PUBLIC -pthread ${Boost_LIBRARIES})

