cmake_minimum_required(VERSION 4.0)

project(
  panlid
  LANGUAGES CXX
)

add_compile_definitions(GLM_ENABLE_EXPERIMENTAL)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Catch2 3 REQUIRED)
find_package(glm REQUIRED)
find_package(magic_enum REQUIRED)
find_package(unofficial-shaderc REQUIRED)
find_package(SDL3 REQUIRED)
find_package(SDL3_mixer REQUIRED)

include(${CMAKE_SOURCE_DIR}/cmake/project_helpers.cmake)
enable_testing()

add_subdirectory(src)
add_subdirectory(tests)

add_subdirectory(examples)
