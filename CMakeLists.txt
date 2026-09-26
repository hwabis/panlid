cmake_minimum_required(VERSION 4.0)

project(
  wonton
  LANGUAGES CXX
)

add_compile_definitions(GLM_ENABLE_EXPERIMENTAL)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(${CMAKE_SOURCE_DIR}/cmake/project_helpers.cmake)
enable_testing()

find_package(glm REQUIRED)
find_package(GTest REQUIRED)
find_package(SDL3 REQUIRED)

add_subdirectory(src)
add_subdirectory(tests)
