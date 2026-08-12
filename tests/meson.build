gtest_main_dep = subproject('gtest').get_variable('gtest_main_dep')

test_executable = executable(
  'run-tests',
  [
    'test-intersection.cpp',
    '../common/intersection.cpp',

    'test-region.cpp',
    '../core/region.cpp',
  ],
  dependencies: [
    dependency('glm', static: true),
    dependency('wayland-server', static: true),
    gtest_main_dep,
  ],
  include_directories: include_directories('..'),
)

test('run-tests', test_executable)
