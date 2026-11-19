wayland_scanner_dep = dependency('wayland-scanner', native: true)
scanner             = find_program(wayland_scanner_dep.get_variable('wayland_scanner'))

wayland_protocols_dep      = dependency('wayland-protocols')
wayland_protocols_base_dir = wayland_protocols_dep.get_variable('pkgdatadir')

foreach name, path : {
  'pointer-constraints': join_paths(wayland_protocols_base_dir,  'unstable/pointer-constraints/pointer-constraints-unstable-v1.xml'),
  'text-input'         : join_paths(wayland_protocols_base_dir,  'unstable/text-input/text-input-unstable-v3.xml'),
  'wl-workspace'       : join_paths(wayland_protocols_base_dir,  'staging/ext-workspace/ext-workspace-v1.xml'),
  'xdg-foreign'        : join_paths(wayland_protocols_base_dir,  'unstable/xdg-foreign/xdg-foreign-unstable-v2.xml'),
  'xdg-output'         : join_paths(wayland_protocols_base_dir,  'unstable/xdg-output/xdg-output-unstable-v1.xml'),
  'xdg-shell'          : join_paths(wayland_protocols_base_dir,  'stable/xdg-shell/xdg-shell.xml'),
  'input-method'       : join_paths(meson.project_source_root(), '3rdparty/wlroots/protocol/input-method-unstable-v2.xml'),
  'virtual-keyboard'   : join_paths(meson.project_source_root(), '3rdparty/wlroots/protocol/virtual-keyboard-unstable-v1.xml'),
  'wlr-layer-shell'    : join_paths(meson.project_source_root(), '3rdparty/wlr-protocols/unstable/wlr-layer-shell-unstable-v1.xml'),
  'zwin'               : join_paths(meson.project_source_root(), '3rdparty/zwin/protocol/zwin.xml'),
  'zwin-shell'         : join_paths(meson.project_source_root(), '3rdparty/zwin/protocol/zwin-shell.xml'),
  'zwin-gles-v32'      : join_paths(meson.project_source_root(), '3rdparty/zwin/protocol/zwin-gles-v32.xml'),
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
inc += include_directories('.', is_system: true)
