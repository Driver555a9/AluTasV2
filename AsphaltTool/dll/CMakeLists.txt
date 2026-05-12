file(GLOB_RECURSE DLL_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
)

add_library(AsphaltToolDLL SHARED ${DLL_SOURCES})

target_include_directories(AsphaltToolDLL PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../shared/src 
    ${CMAKE_CURRENT_SOURCE_DIR}/include 
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_directories(AsphaltToolDLL PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/lib
)

target_link_libraries(AsphaltToolDLL PUBLIC
    Shared
    libMinHook.x64.dll
)