project('yabe-compositor', 'cpp', 'c',
  version: '0.1.0',
  default_options: [
    'warning_level=3',
    'cpp_std=c++23',
    'default_library=static',
  ]
)

libffi_sp            = subproject('libffi', default_options: ['tests=false'])
wayland_sp           = subproject('wayland', default_options: ['tests=false', 'documentation=false', 'scanner=false', 'dtd_validation=false', 'default_library=static'])
wayland_protocols_sp = subproject('wayland-protocols', default_options: ['tests=false'])
glm_sp              = subproject('glm')

srcs = []
subdir('common')
subdir('core')
subdir('protocols')

yabe_compositor_lib = static_library('yabe_compositor',
  srcs,
  dependencies: [
    dependency('wayland-server', static: true),
    dependency('glm', static: true),
  ],
  link_whole: [
    wayland_sp.get_variable('wayland_server'),
    libffi_sp.get_variable('ffi_lib'),
  ],
)
