cmake_minimum_required(VERSION 2.8)

project(teensylcd CXX C)

# pull in modules
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMakeModules/")

# handle clang
if("${CMAKE_CXX_COMPILER}" MATCHES "clang(\\+\\+)?$" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR APPLE)
    set(CMAKE_COMPILER_IS_CLANGXX 1)
endif()

# enable c99/ c++11
if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_CLANGXX OR EMSCRIPTEN)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif()

# enable all warnings
if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_CLANGXX OR EMSCRIPTEN)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
endif()

# set cflags that are common to dependancies
add_definitions(-D_FILE_OFFSET_BITS=64)
add_definitions(-D_GNU_SOURCE)

# update cflags for debug build
if(CMAKE_BUILD_TYPE MATCHES "Debug")
    add_definitions(-D_DEBUG)
    message("Building debug executable.")
endif()

# kill annoying clang warnings
if(CMAKE_COMPILER_IS_CLANGXX)
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Qunused-arguments")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Qunused-arguments")
endif()

# same for clang/emscripten
if(EMSCRIPTEN)
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-warn-absolute-paths")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-warn-absolute-paths")
endif()

# build simavr
add_subdirectory(simavr)

# build libteensylcd
add_subdirectory(libteensylcd)

# build teensyrun on desktop, teensyweb otherwise
if(NOT EMSCRIPTEN)
    add_subdirectory(teensylcd-run)
else()
    add_subdirectory(teensylcd-web)
endif()

