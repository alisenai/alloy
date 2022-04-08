set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_GLIBCXX_DEBUG -g -O0 -fsanitize=address -fno-omit-frame-pointer")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

add_executable(AlloyTesting)

add_subdirectory(Libraries)
add_subdirectory(Source)

target_link_libraries(AlloyTesting PUBLIC AlloyTestingLibraries Alloy)
