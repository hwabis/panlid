add_library(panlid
  graphics/Colour4.cc
  logging/Logger.cc
)

set_target_defaults(panlid)

target_include_directories(panlid
  PUBLIC ${CMAKE_SOURCE_DIR}/include
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(panlid
  PUBLIC
    SDL3::SDL3
    glm::glm
    magic_enum::magic_enum
)

target_compile_features(panlid PUBLIC cxx_std_20)
