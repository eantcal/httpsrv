cmake_minimum_required(VERSION 2.8.12)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wno-missing-field-initializers")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O2")

include_directories(. include 3pp/PicoSHA2 3pp/zip/src )

file(GLOB SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc" "${CMAKE_CURRENT_SOURCE_DIR}/3pp/zip/src/*.c" )

add_executable(httpsrv ${SOURCES})
set_target_properties(httpsrv PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS ON
)


target_link_libraries(httpsrv LINK_PUBLIC -pthread ${Boost_LIBRARIES})

