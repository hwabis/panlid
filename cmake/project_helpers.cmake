include(GoogleTest)

function(set_target_defaults target)
  set_target_properties(${target} PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    COMPILE_WARNING_AS_ERROR ON
  )

  target_compile_options(${target} PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
  )
endfunction()

function(add_test_executable target test_lib)
  if(NOT ARGN)
    message(FATAL_ERROR
      "add_test_executable called without any source files "
      "for target: ${target}"
    )
  endif()

  add_executable(${target} ${ARGN})
  set_target_defaults(${target})
  target_link_libraries(${target} PRIVATE
    ${test_lib}
    GTest::gtest_main
  )

  gtest_discover_tests(${target})
endfunction()
