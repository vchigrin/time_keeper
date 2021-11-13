db_entities = library('db_entities',
                      'database.cc',
                      'database.h',
                      'error_codes.cc',
                      'error_codes.h',
                      'select_rows.cc',
                      'select_rows.h',
                      'task.cc',
                      'task.h',
                      'verify.h',
                      include_directories : [project_include_dir],
                      dependencies : [
                        sqlite_dep,
                        boost_dep,
                      ])

executable('mobile_time_tracker',
           'main.cc',
           install : true,
           include_directories : [project_include_dir],
           link_with: db_entities,
           dependencies : [
             gtkmm_dep,
             libhandy_dep,
           ])

tests_executable = executable('tests_executable',
           'db_entities_test.cc',
           include_directories : [project_include_dir],
           link_with: db_entities,
           dependencies : [
               gtest_dep,
               gtest_main_dep,
           ])

test('tests', tests_executable)
