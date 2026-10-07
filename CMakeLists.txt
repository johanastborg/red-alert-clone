cmake_minimum_required(VERSION 3.16)
project(RedAlertSovietClone LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GLFW REQUIRED glfw3)
pkg_check_modules(GLM REQUIRED glm)

find_library(OPENGL_LIB OpenGL REQUIRED)
find_library(OPENAL_LIB OpenAL REQUIRED)

include_directories(
    src
    ${GLFW_INCLUDE_DIRS}
    ${GLM_INCLUDE_DIRS}
)

link_directories(
    ${GLFW_LIBRARY_DIRS}
)

set(SOURCES
    src/Main.cpp
    src/AudioSystem.cpp
    src/TextureAtlas.cpp
    src/Renderer.cpp
    src/Map.cpp
    src/Entities.cpp
    src/Combat.cpp
    src/AI.cpp
    src/UI.cpp
    src/Game.cpp
)

add_executable(redalert ${SOURCES})

target_compile_options(redalert PRIVATE -Wall -Wextra -Wno-deprecated-declarations -O2)

target_link_libraries(redalert PRIVATE
    ${GLFW_LIBRARIES}
    ${OPENGL_LIB}
    ${OPENAL_LIB}
)
