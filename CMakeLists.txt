cmake_minimum_required(VERSION 2.8)
project(backend-ctp)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread")

include_directories("api")
include_directories("src")

set(SOURCE_FILES src/main.cpp src/MdSpi.cpp src/TraderSpi.cpp src/global.cpp src/jsoncpp.cpp)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
add_executable(backend-ctp ${SOURCE_FILES})
target_link_libraries(backend-ctp ev hiredis redox thostmduserapi thosttraderapi)
