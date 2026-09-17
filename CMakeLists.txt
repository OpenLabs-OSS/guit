cmake_minimum_required(VERSION 3.21)

project(Guit
    VERSION 0.1.0
    DESCRIPTION "A beginner-friendly desktop Git client that shows what Git is doing"
    LANGUAGES CXX
)

option(GUIT_BUILD_TESTS "Build the Guit test suite" ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# The primary Windows compiler is MinGW-w64 (MSVC is not used).
find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets)

if(GUIT_BUILD_TESTS)
    find_package(Qt6 REQUIRED COMPONENTS Test)
    enable_testing()
endif()

add_subdirectory(src)

if(GUIT_BUILD_TESTS)
    add_subdirectory(tests)
endif()
