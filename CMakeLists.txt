cmake_minimum_required(VERSION 3.16)
project(RedWire2)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/RedWire2/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/RedWire2/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/RedWire2/bin)

add_subdirectory(ext/SFML)

add_executable(RedWire2
        src/main.cpp
        src/Core/Layer.cpp
        src/Interface/LayerView.cpp
        )

target_include_directories(RedWire2 PRIVATE src)

target_link_libraries(RedWire2
        sfml-system
        sfml-window
        sfml-graphics
        )
