project('mobile_time_tracker', 'cpp',
  version : '0.1',
  default_options : ['warning_level=3', 'cpp_std=c++17'])


gtkmm_dep = dependency('gtkmm-3.0')
libhandy_dep = dependency('libhandy-1')
sqlite_dep = dependency('sqlite3')
boost_dep = dependency('boost')

project_include_dir = include_directories('.')

subdir('app')
