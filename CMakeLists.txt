cmake_minimum_required(VERSION 3.19)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(imageviewergl CXX)

find_package(glfw3 3.3.4 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(OpenGL REQUIRED)

file(GLOB cpps src/*.cpp)
add_executable(${PROJECT_NAME} ${cpps})

target_include_directories(${PROJECT_NAME} PRIVATE sub/stb)
target_include_directories(${PROJECT_NAME} PRIVATE util)

target_link_libraries(${PROJECT_NAME} "glm::glm;glfw;OpenGL::GL")

#target_compile_definitions(${PROJECT_NAME} PUBLIC TRANSPARENT_WINDOW )
