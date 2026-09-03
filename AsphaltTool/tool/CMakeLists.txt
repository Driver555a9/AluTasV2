file(GLOB_RECURSE TOOL_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
)

add_executable(AsphaltRuntimeToolkit ${TOOL_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/asphaltruntimetool.rc) 

target_include_directories(AsphaltRuntimeToolkit PUBLIC
    ${CMAKE_SOURCE_DIR}/engine/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../shared/src
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_directories(AsphaltRuntimeToolkit PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/lib
)

target_link_libraries(AsphaltRuntimeToolkit PUBLIC
    Core
    Shared
    libmem.lib
    ntdll.lib
)