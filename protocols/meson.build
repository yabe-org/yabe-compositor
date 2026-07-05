wayland_scanner_dep = dependency('wayland-scanner', native: true)
scanner             = find_program(wayland_scanner_dep.get_variable('wayland_scanner'))

wayland_protocols_dep      = dependency('wayland-protocols')
wayland_protocols_base_dir = wayland_protocols_dep.get_variable('pkgdatadir')

xdg_shell_xml        = join_paths(wayland_protocols_base_dir, 'stable', 'xdg-shell', 'xdg-shell.xml')
xdg_shell_protocol_h = custom_target(
  'xdg-shell-protocol.h',
  output: 'xdg-shell-protocol.h',
  input: xdg_shell_xml,
  command: [scanner, 'server-header', '@INPUT@', '@OUTPUT@'],
)
xdg_shell_protocol_c = custom_target(
  'xdg-shell-protocol.c',
  output: 'xdg-shell-protocol.c',
  input: xdg_shell_xml,
  command: [scanner, 'private-code', '@INPUT@', '@OUTPUT@'],
)

srcs += [
  'protocols/protocols.cpp',

  'protocols/wayland/compositor.cpp',
  'protocols/wayland/output.cpp',
  'protocols/wayland/region.cpp',
  'protocols/wayland/surface.cpp',
  'protocols/wayland/wayland.cpp',
  'protocols/wayland/seat/seat.cpp',
  'protocols/wayland/seat/pointer.cpp',

  'protocols/xdg-shell/xdg-shell.cpp',
  'protocols/xdg-shell/xdg-surface.cpp',
  'protocols/xdg-shell/xdg-surface-role.cpp',
  'protocols/xdg-shell/xdg-toplevel.cpp',
  'protocols/xdg-shell/xdg-wm-base.cpp',
  xdg_shell_protocol_h,
  xdg_shell_protocol_c,
]

