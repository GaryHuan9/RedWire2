cmake_minimum_required(VERSION 3.20)
project(RedWire2 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)

# RedWire2.Core
add_library(RedWire2.Core "")
add_subdirectory(src)
add_subdirectory(ext)

set_property(TARGET RedWire2.Core PROPERTY CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
target_include_directories(RedWire2.Core PUBLIC include)

# RedWire2.Resources
file(GLOB_RECURSE resources rsc/**/*)

add_custom_command(OUTPUT resources POST_BUILD COMMAND
        ${CMAKE_COMMAND} -E copy_directory_if_different
        ${CMAKE_SOURCE_DIR}/rsc
        ${CMAKE_BINARY_DIR}/rsc
)

add_custom_target(RedWire2.Resources DEPENDS resources)

# RedWire2
add_executable(RedWire2 src/main.cpp)
target_link_libraries(RedWire2 RedWire2.Core)
add_dependencies(RedWire2 RedWire2.Resources)

# Tests
add_executable(RedWire2.Tests "")
add_subdirectory(tests)
