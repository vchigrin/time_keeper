project('mobile_time_tracker', 'cpp',
  version : '0.1',
  default_options : ['warning_level=3', 'cpp_std=c++17', 'werror=true'])


add_global_arguments(
    '-Wconversion',
    '-Werror',  # For some reason werror=true does not work for tests.
    '-Wold-style-cast',
    language : 'cpp')

gtkmm_dep = dependency('gtkmm-3.0')
libhandy_dep = dependency('libhandy-1')
sqlite_dep = dependency('sqlite3')
boost_dep = dependency('boost')
gtest_dep = dependency('gtest')
gtest_main_dep = dependency('gtest_main')

project_include_dir = include_directories('.')

subdir('app')
