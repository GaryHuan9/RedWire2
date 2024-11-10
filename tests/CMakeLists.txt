include(FetchContent)
set(ONLY_LIBS ON)
set(BUILD_SHARED_LIBS ON)

#googletest
fetchcontent_declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.15.2
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
fetchcontent_makeavailable(googletest)
target_link_libraries(RedWire2.Tests PRIVATE GTest::gtest_main)

#RedWire2.Core and tests
target_link_libraries(RedWire2.Tests PRIVATE RedWire2.Core)

target_sources(RedWire2.Tests PRIVATE
        RecyclingListTests.cpp
        WireBridgeTests.cpp
)
