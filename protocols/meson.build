wayland_scanner_dep = dependency('wayland-scanner', native: true)
scanner             = find_program(wayland_scanner_dep.get_variable('wayland_scanner'))

wayland_protocols_dep      = dependency('wayland-protocols')
wayland_protocols_base_dir = wayland_protocols_dep.get_variable('pkgdatadir')

foreach name, path : {
  'xdg-shell'      : join_paths(wayland_protocols_base_dir,  'stable/xdg-shell/xdg-shell.xml'),
  'xdg-output'     : join_paths(wayland_protocols_base_dir,  'unstable/xdg-output/xdg-output-unstable-v1.xml'),
  'wlr-layer-shell': join_paths(meson.project_source_root(), '3rdparty/wlr-protocols/unstable/wlr-layer-shell-unstable-v1.xml'),
  'zwin'           : join_paths(meson.project_source_root(), '3rdparty/zwin/protocol/zwin.xml'),
  'zwin-shell'     : join_paths(meson.project_source_root(), '3rdparty/zwin/protocol/zwin-shell.xml'),
  'zwin-gles-v32'  : join_paths(meson.project_source_root(), '3rdparty/zwin/protocol/zwin-gles-v32.xml'),
}
  filename = name + '-protocol'
  srcs += custom_target(
    filename + '.c',
    output: filename + '.c',
    input: path,
    command: [scanner, 'private-code', '@INPUT@', '@OUTPUT@'],
  )
  # wlr-layer-shell uses arg name `namespace`, which is reserved by C++
  if name == 'wlr-layer-shell'
    tmp_name = filename + '.h_tmp'
    header = custom_target(
      tmp_name,
      output: tmp_name,
      input: path,
      command: [scanner, 'server-header', '@INPUT@', '@OUTPUT@'],
    )
    srcs += custom_target(
      filename + '.h',
      output: filename + '.h',
      input: header,
      command: [
        'sh', '-c',
        'sed "s/namespace/name_space/g" $1 > $2',
        'sh', # $0
        '@INPUT@',
        '@OUTPUT@',
      ],
    )
  else
    srcs += custom_target(
      filename + '.h',
      output: filename + '.h',
      input: path,
      command: [scanner, 'server-header', '@INPUT@', '@OUTPUT@'],
    )
  endif
endforeach

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

  'wlr/layer-shell.cpp',
  'wlr/layer-surface.cpp',
  'wlr/wlr.cpp',

  'xdg/shell/xdg-popup.cpp',
  'xdg/shell/xdg-positioner.cpp',
  'xdg/shell/xdg-surface.cpp',
  'xdg/shell/xdg-toplevel.cpp',
  'xdg/shell/xdg-wm-base.cpp',
  'xdg/output/xdg-output-manager.cpp',
  'xdg/output/xdg-output.cpp',
  'xdg/xdg.cpp',

  'zwin/gles-v32/base-technique.cpp',
  'zwin/gles-v32/buffer.cpp',
  'zwin/gles-v32/gles-v32.cpp',
  'zwin/gles-v32/program.cpp',
  'zwin/gles-v32/rendering-unit.cpp',
  'zwin/gles-v32/sampler.cpp',
  'zwin/gles-v32/shader.cpp',
  'zwin/gles-v32/texture.cpp',
  'zwin/gles-v32/vertex-array.cpp',
  'zwin/shell/bounded.cpp',
  'zwin/shell/shell.cpp',
  'zwin/shm/shm-buffer.cpp',
  'zwin/shm/shm-pool.cpp',
  'zwin/shm/shm.cpp',
  'zwin/compositor.cpp',
  'zwin/ray.cpp',
  'zwin/region.cpp',
  'zwin/seat.cpp',
  'zwin/virtual-object.cpp',
  'zwin/zwin.cpp',
)
