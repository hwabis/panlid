find_program(GLSLC glslc REQUIRED)

set(spike_shaders anim.vert anim.frag)
foreach(shader IN LISTS spike_shaders)
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${shader}.spv
    COMMAND ${GLSLC} ${CMAKE_CURRENT_SOURCE_DIR}/${shader} -o ${CMAKE_CURRENT_BINARY_DIR}/${shader}.spv
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${shader})
  list(APPEND spike_spv ${CMAKE_CURRENT_BINARY_DIR}/${shader}.spv)
endforeach()
add_custom_target(spike-shaders DEPENDS ${spike_spv})

add_executable(spike-window spike_window.cc)
set_target_defaults(spike-window)
add_dependencies(spike-window spike-shaders)
target_link_libraries(spike-window PRIVATE panlid SDL3::SDL3)
target_compile_definitions(spike-window PRIVATE SPIKE_SHADER_DIR="${CMAKE_CURRENT_BINARY_DIR}")
