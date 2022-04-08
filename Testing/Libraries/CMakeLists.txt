add_library(AlloyTestingLibraries INTERFACE)

add_subdirectory(External)
#add_subdirectory(Internal)

target_include_directories(AlloyTestingLibraries INTERFACE External #[[Internal]])
target_link_libraries(AlloyTestingLibraries INTERFACE #[[AlloyTestingLibrariesInternal]] AlloyTestingLibrariesExternal)
