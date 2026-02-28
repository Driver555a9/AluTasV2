cmake_minimum_required(VERSION 3.20)
project(AsphaltTool)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(engine)

file(GLOB_RECURSE APP_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
)

set_property(DIRECTORY PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/**/*.cpp"
)

add_executable(AsphaltTas ${APP_SOURCES})

target_include_directories(AsphaltTas PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_directories(AsphaltTas PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/lib
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/bullet
)

target_link_libraries(AsphaltTas PUBLIC
    Core
    libmem
    ntdll
)

target_compile_options(AsphaltTas PUBLIC
    -Wall -Wextra -Wpedantic -O2 -g
)

target_link_options(AsphaltTas PUBLIC
    -static
    -static-libgcc
    -static-libstdc++
)