gnome = import('gnome')

mobild_time_tracker_resources = gnome.compile_resources(
   'm-time-tracker-resources',
   'm-time-tracker.gresources.xml',
)

resources_lib = static_library('resources_lib',
                        mobild_time_tracker_resources,
                        cpp_args: ['-Wno-old-style-cast'],
                        dependencies : [
                          gtkmm_dep,
                        ])

db_entities = static_library('db_entities',
                      'activity.cc',
                      'activity.h',
                      'database.cc',
                      'database.h',
                      'error_codes.cc',
                      'error_codes.h',
                      'select_rows.cc',
                      'select_rows.h',
                      'task.cc',
                      'task.h',
                      'utils.cc',
                      'utils.h',
                      'verify.cc',
                      'verify.h',
                      include_directories : [project_include_dir],
                      dependencies : [
                        sqlite_dep,
                        boost_dep,
                      ])

executable('mobile_time_tracker',
           'app_state.cc',
           'app_state.h',
           'edit_task_dialog.cc',
           'edit_task_dialog.h',
           'list_model_base.h',
           'main.cc',
           'main_window.cc',
           'main_window.h',
           'recent_activities_model.cc',
           'recent_activities_model.h',
           'task_list_model_base.cc',
           'task_list_model_base.h',
           'ui_helpers.h',
           install : true,
           gui_app: true,
           include_directories : [project_include_dir],
           link_with: db_entities,
           # link_whole to prevent linker throw away contents of the resource
           # archive - we interested in "constructor" function that registers
           # resources in it.
           link_whole: resources_lib,
           dependencies : [
             gtkmm_dep,
             libhandy_dep,
           ])

tests_executable = executable('tests_executable',
           'db_entities_test.cc',
           'utils_test.cc',
           include_directories : [project_include_dir],
           link_with: db_entities,
           dependencies : [
               gtest_dep,
               gtest_main_dep,
           ])

test('tests', tests_executable)
