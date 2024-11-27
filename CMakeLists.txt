cmake_minimum_required(VERSION 3.26)
project(RedWire2 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)

# RedWire2.Core
add_library(RedWire2.Core "")
add_subdirectory(src)
add_subdirectory(ext)

set_property(TARGET RedWire2.Core PROPERTY CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
target_include_directories(RedWire2.Core PUBLIC include)
target_include_directories(RedWire2.Core PUBLIC src)

# RedWire2.Resources
add_library(RedWire2.Resources INTERFACE)
add_subdirectory(rsc)

# RedWire2
add_executable(RedWire2 src/main.cpp)
target_link_libraries(RedWire2 RedWire2.Core)
add_dependencies(RedWire2 RedWire2.Resources)

# RedWire2.Tests
add_executable(RedWire2.Tests "")
add_subdirectory(tests)
