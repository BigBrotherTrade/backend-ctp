# Copyright 2016 timercrack
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
project(backend-ctp LANGUAGES CXX)
cmake_minimum_required(VERSION 3.2)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DELPP_STACKTRACE_ON_CRASH -DELPP_THREAD_SAFE -DJSON_DIAGNOSTICS")
include_directories("api")
include_directories("src")
if (WIN32)
    include_directories("api/win")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_CRT_SECURE_NO_WARNINGS")
    add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
    find_library(HIREDIS hiredis_static HINT "api/win")
    find_library(REDIS++ redis++_static HINT "api/win")
    find_library(WINDATACOLLECT WinDataCollect HINT "api/win")
    find_library(MDAPI thostmduserapi_se HINT "api/win")
    find_library(TRADERAPI thosttraderapi_sm HINT "api/win")
else()
    include_directories("api/linux")
    SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g -ggdb3")
    SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
    find_library(HIREDIS hiredis)
    find_library(REDIS++ redis++)
    find_library(LINUXDATACOLLECT LinuxDataCollect)
    find_library(MDAPI thostmduserapi)
    find_library(TRADERAPI thosttraderapi)
    find_package(fmt)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY /usr/local/bin/)
endif()
set(SOURCE_FILES src/main.cpp src/MdSpi.cpp src/TraderSpi.cpp src/global.cpp src/easylogging++.cc)
add_executable(${PROJECT_NAME} ${SOURCE_FILES})
find_package(Threads REQUIRED)
if (WIN32)
    target_link_libraries(${PROJECT_NAME} ${WINDATACOLLECT} ws2_32)
else()
    target_link_libraries(${PROJECT_NAME} ${LINUXDATACOLLECT})
    target_link_libraries(${PROJECT_NAME} fmt::fmt)
endif()
target_link_libraries(${PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT} ${HIREDIS} ${REDIS++} ${MDAPI} ${TRADERAPI})
