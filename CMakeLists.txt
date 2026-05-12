cmake_minimum_required(VERSION 4.1)
project(AsphaltTool LANGUAGES C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/Output")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/Output")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/Output")

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")

add_subdirectory(engine)
add_subdirectory(AsphaltTool/dll)
add_subdirectory(AsphaltTool/shared)
add_subdirectory(AsphaltTool/tool)