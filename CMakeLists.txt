cmake_minimum_required(VERSION 3.20)
project(RedWire2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/RedWire2/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/RedWire2/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/RedWire2/bin)

add_executable(RedWire2 "")
add_subdirectory(ext)
add_subdirectory(src)

set_property(TARGET RedWire2 PROPERTY CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
target_include_directories(RedWire2 PRIVATE include)

add_custom_command(TARGET RedWire2 POST_BUILD COMMAND
        ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/ext/JetBrainsMono
        ${CMAKE_BINARY_DIR}/cubes/ext/JetBrainsMono
)
