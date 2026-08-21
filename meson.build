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
xkbcommon_sp         = subproject('libxkbcommon', default_options: ['tests=false', 'enable-tools=false', 'enable-x11=false', 'enable-docs=false', 'enable-xkbregistry=false', 'enable-bash-completion=false', 'default_library=static'])
lua_sp               = subproject('lua', default_options: ['default_library=static', 'loadlib=enabled', 'interpreter=false'])

srcs = [
  files('yabe-compositor.cpp'),
]
subdir('common')
subdir('core')
subdir('protocols')

deps = [
    dependency('wayland-server', static: true),
    dependency('glm', static: true),
    dependency('xkbcommon', static: true),
    dependency('lua', static: true),
]
yabe_compositor_lib = static_library('yabe_compositor',
  srcs,
  dependencies: deps,
  link_whole: [
    wayland_sp.get_variable('wayland_server'),
    libffi_sp.get_variable('ffi_lib'),
    xkbcommon_sp.get_variable('libxkbcommon'),
    lua_sp.get_variable('lua_lib'),
  ],
)

# `srcs` has file pathes related to root directory,
# but they cannot be resolved from tools/,
# so convert here by files()
# Note that files() does not support generated files
# setting_meta_srcs = [files(srcs), generated_srcs]
subdir('tools')

if get_option('tests')
  subdir('tests')
endif
