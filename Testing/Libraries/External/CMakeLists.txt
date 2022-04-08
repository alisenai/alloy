add_library(DeltaTesterLibrariesExternal INTERFACE)

add_subdirectory(googletest)
enable_testing()
include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})
option(BENCHMARK_ENABLE_TESTING "Enable testing of the benchmark library." ON)
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "Enable testing of the benchmark library." FORCE)
add_subdirectory(benchmark)

target_link_libraries(DeltaTesterLibrariesExternal INTERFACE gtest benchmark::benchmark)
