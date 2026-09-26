add_library(wonton
  hi.cc
)

set_target_defaults(wonton)

target_include_directories(wonton
  PUBLIC ${CMAKE_SOURCE_DIR}/include
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(wonton
  PUBLIC
    SDL3::SDL3
    glm::glm
)
