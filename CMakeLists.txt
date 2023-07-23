cmake_minimum_required(VERSION 3.19)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Tells CMake to generate compile_commands.json which is necessary for proper Intellisense in VSCode.
# But that json is only generated if using either the Makefile or Ninja generator.
# In Windows you can tell CMake to use Ninja by creating an environment variable CMAKE_GENERATOR and setting it to Ninja.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(imageviewergl CXX)

# NOTICE: on Windows in CLion I was using vcpkg to manage my libraries,
# and my CLion CMake settings for this project look like this:
#
# File / Settings / Build, Execution, Deployment / CMake / (Profile) / CMake options:
#    -GNinja -DCMAKE_TOOLCHAIN_FILE=c:/vcpkg/scripts/buildsystems/vcpkg.cmake
#
# NOTE: VScode with vcpkg
#   make sure you have this in your vscode settings.json file:
# {
#   ...
#   "cmake.configureSettings": {
#     "CMAKE_TOOLCHAIN_FILE": "C:/vcpkg/scripts/buildsystems/vcpkg.cmake"
#   },
#   ...
# }
#
# NOTICE: if on Windows using vcpkg and CMake suddenly can't find anything,
# consider reinstalling vcpkg (delete vcpkg directory, follow instructions online to re-install).
# No idea why, but mine stopped working and reinstalling it was the only way to get it working again.
# This also means using vcpkg to reinstall all dependencies.
#
# TODO: It is possible to install vcpkg as a git (submodule?) into this project folder, and to
#   modify this CMakeLists.txt to bootstrap it as needed. I don't feel like figuring that out right now,
#   but I've done it on other projects so I remember that it's possible and not too difficult.

find_package(glfw3 3.3.4 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)

file(GLOB cpps src/*.cpp util/*.cpp)

add_executable(${PROJECT_NAME} ${cpps})

target_include_directories(${PROJECT_NAME} PRIVATE sub/stb)
target_include_directories(${PROJECT_NAME} PRIVATE util)

target_link_libraries(${PROJECT_NAME} PRIVATE "glm::glm;glfw;OpenGL::GL;GLEW::GLEW")

#target_compile_definitions(${PROJECT_NAME} PUBLIC TRANSPARENT_WINDOW )
