project('yabe-compositor', 'cpp', 'c',
  version: '0.1.0',
  default_options: [
    'warning_level=3',
    'cpp_std=c++23',
    'default_library=static',
  ]
)
add_project_arguments([
  '-DGLM_ENABLE_EXPERIMENTAL',
  '-Wno-gnu-zero-variadic-macro-arguments',
], language: 'cpp')

libffi_sp            = subproject('libffi', default_options: ['tests=false'])
wayland_sp           = subproject('wayland', default_options: ['tests=false', 'documentation=false', 'scanner=false', 'dtd_validation=false', 'default_library=static'])
wayland_protocols_sp = subproject('wayland-protocols', default_options: ['tests=false'])
glm_sp               = subproject('glm')
xkbcommon_sp         = subproject('libxkbcommon', default_options: ['enable-tools=false', 'enable-x11=false', 'enable-docs=false', 'enable-xkbregistry=false', 'enable-bash-completion=false', 'default_library=static'])

srcs = [
  'yabe-compositor.cpp',
]
subdir('common')
subdir('core')
subdir('protocols')

yabe_compositor_lib = static_library('yabe_compositor',
  srcs,
  dependencies: [
    dependency('wayland-server', static: true),
    dependency('glm', static: true),
    dependency('xkbcommon', static: true),
  ],
  link_whole: [
    wayland_sp.get_variable('wayland_server'),
    libffi_sp.get_variable('ffi_lib'),
    xkbcommon_sp.get_variable('libxkbcommon'),
  ],
)

if get_option('tests')
  subdir('tests')
endif
