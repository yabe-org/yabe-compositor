generate_meta_file   = get_option('generate-setting-meta-file')
generate_reference   = get_option('generate-setting-reference-file')
if generate_meta_file or generate_reference
  generate_setting_meta_file = executable(
    'generate-setting-meta-file',
    [
      srcs,
      'generate-setting-meta-file.cpp',
      'json.cpp',
      'lua-meta-file.cpp',
      'reference-json.cpp',
    ],
    cpp_args: '-DYABE_SETTING_ENABLE_META_FILE_GENERATION',
    dependencies: [
      deps,
      dependency('gl'),
    ],
    include_directories: '..',
  )
endif

if generate_meta_file
  custom_target(
    'yabe.meta.lua',
    output: 'yabe.meta.lua',
    command: [generate_setting_meta_file, '--format=lua'],
    capture: true,
    build_by_default: true,
  )
endif

if generate_reference
  custom_target(
    'yabe.reference.json',
    output: 'yabe.reference.json',
    command: [generate_setting_meta_file, '--format=json'],
    capture: true,
    build_by_default: true,
  )
endif
