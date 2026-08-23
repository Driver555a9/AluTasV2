file(GLOB_RECURSE TOOL_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
)

add_executable(AsphaltTas ${TOOL_SOURCES})


target_include_directories(AsphaltTas PUBLIC
    ${CMAKE_SOURCE_DIR}/engine/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../shared/src
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_directories(AsphaltTas PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/lib
)

target_link_libraries(AsphaltTas PUBLIC
    Core
    Shared
    libmem.lib
    ntdll.lib
)