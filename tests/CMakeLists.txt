add_subdirectory(googletest)
include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})

target_sources(RedWire2.Tests PRIVATE
        RecyclingListTests.cpp
        WireBridgeTests.cpp
)

target_link_libraries(RedWire2.Tests RedWire2.Core)
target_link_libraries(RedWire2.Tests gtest gtest_main)
