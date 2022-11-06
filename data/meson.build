desktop_file = i18n.merge_file(
    input: meson.project_name() + '.desktop.in',
    output: meson.project_name() + '.desktop',
    type: 'desktop',
    po_dir: '../po',
    install: true,
    install_dir: join_paths(get_option('datadir'), 'applications')
)

install_data(
    meson.project_name() + '.svg',
    install_dir: join_paths(
        get_option('datadir'), 'icons', 'hicolor', 'scalable', 'apps')
)

desktop_file_validate = find_program('desktop-file-validate')

test(
    'Validate desktop file',
     desktop_file_validate,
     args: [desktop_file]
)

appstream_file = i18n.merge_file(
  input: 'io.github.vchigrin.time_keeper.appdata.xml.in',
  output: 'io.github.vchigrin.time_keeper.appdata.xml',
  po_dir: '../po',
  install: true,
  install_dir: join_paths(get_option('datadir'), 'appdata')
)

appstream_util = find_program('appstream-util')
test(
    'Validate appstream file',
    appstream_util,
    args: ['validate', appstream_file]
)
