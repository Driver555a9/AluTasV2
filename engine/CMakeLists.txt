cmake_minimum_required(VERSION 3.20)
project(CoreEngine LANGUAGES C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")

file(GLOB_RECURSE CORE_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
)

add_library(Core STATIC ${CORE_SOURCES})

# target_compile_options(Core PRIVATE /Zi /Od /RTC1) # disables optimization

target_include_directories(Core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/dependencies
    ${CMAKE_CURRENT_SOURCE_DIR}/include/dependencies/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/include/dependencies/stb_image
    ${CMAKE_CURRENT_SOURCE_DIR}/include/dependencies/bullet
)


target_link_directories(Core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/lib
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/bullet
)

target_link_libraries(Core PUBLIC

    # --- Assimp / GLFW / ENet / ImGui / Zlib ---
    assimp.lib
    glfw3.lib
    imgui.lib
    enet.lib
    zlibstatic.lib

    # --- Bullet ---
    LinearMath.lib
    BulletDynamics.lib
    BulletCollision.lib

    # --- Windows system libs ---
    ole32
    uuid
    shell32
    user32
    gdi32
    kernel32
    opengl32
    ws2_32
    winmm
    dwmapi
)