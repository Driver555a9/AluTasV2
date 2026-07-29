add_library(Shared INTERFACE)

target_include_directories(Shared INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/include 
)