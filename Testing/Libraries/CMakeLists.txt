add_library(DeltaTesterLibraries INTERFACE)

add_subdirectory(External)
#add_subdirectory(Internal)

target_include_directories(DeltaTesterLibraries INTERFACE External #[[Internal]])
target_link_libraries(DeltaTesterLibraries INTERFACE #[[DeltaTesterLibrariesInternal]] DeltaTesterLibrariesExternal)
