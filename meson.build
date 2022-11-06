project('time_keeper', 'cpp',
  version : '0.1',
  default_options : ['warning_level=3', 'cpp_std=c++17', 'werror=true'])
i18n = import('i18n')


add_global_arguments(
    '-Wconversion',
    '-Werror',  # For some reason werror=true does not work for tests.
    '-Wold-style-cast',
    language : 'cpp')

gettext_def = '-DGETTEXT_PACKAGE="@0@"'.format(meson.project_name())
localedir_def = '-DLOCALE_DIR="@0@/@1@"'.format(
    get_option('prefix'), get_option('localedir'))
add_project_arguments(gettext_def, language:'cpp')
add_project_arguments(localedir_def, language:'cpp')

gtkmm_dep = dependency('gtkmm-3.0')
libhandy_dep = dependency('libhandy-1')
sqlite_dep = dependency('sqlite3')
boost_dep = dependency('boost')
gtest_dep = dependency('gtest')
gtest_main_dep = dependency('gtest_main')

project_include_dir = include_directories('.')

subdir('app')
subdir('po')
