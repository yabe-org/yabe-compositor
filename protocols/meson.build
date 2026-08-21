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

srcs += files(
  'protocols.cpp',

  'wayland/compositor.cpp',
  'wayland/output.cpp',
  'wayland/region.cpp',
  'wayland/surface.cpp',
  'wayland/wayland.cpp',
  'wayland/data-device/data-device-manager.cpp',
  'wayland/data-device/data-device.cpp',
  'wayland/data-device/data-offer.cpp',
  'wayland/data-device/data-source.cpp',
  'wayland/seat/seat.cpp',
  'wayland/seat/pointer.cpp',
  'wayland/seat/keyboard.cpp',
  'wayland/subcompositor/subcompositor.cpp',
  'wayland/subcompositor/subsurface.cpp',

  'xdg-shell/xdg-popup.cpp',
  'xdg-shell/xdg-positioner.cpp',
  'xdg-shell/xdg-shell.cpp',
  'xdg-shell/xdg-surface.cpp',
  'xdg-shell/xdg-toplevel.cpp',
  'xdg-shell/xdg-wm-base.cpp',
)

srcs += [
  xdg_shell_protocol_h,
  xdg_shell_protocol_c,
]
