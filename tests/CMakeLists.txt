add_test_executable(panlid-tests panlid
  bindables/BindableBindingTests.cc
  bindables/BindableLeasingTests.cc
  bindables/BindableTests.cc
  clocks/DecouplingFramedClockTests.cc
  clocks/InterpolatingFramedClockTests.cc
  clocks/StopwatchClockTests.cc
  graphics/TripleBufferTests.cc
  graphics/ColourTests.cc
  graphics/shaders/ShaderCompilerTest.cc
  primitives/QuadTests.cc
  threading/SchedulerTests.cc
  utils/InterpolationTests.cc
)
