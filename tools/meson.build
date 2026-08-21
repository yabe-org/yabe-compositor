if get_option('generate-setting-meta-file')
  generate_setting_meta_file = executable(
    'generate-setting-meta-file',
    [
      srcs,
      'generate-setting-meta-file.cpp',
    ],
    cpp_args: '-DYABE_SETTING_ENABLE_META_FILE_GENERATION',
    dependencies: [
      deps,
      dependency('gl'),
    ],
    include_directories: '..',
  )
  custom_target(
    'yabe.meta.lua',
    output: 'yabe.meta.lua',
    command: generate_setting_meta_file,
    capture: true,
    build_by_default: true,
  )
endif
