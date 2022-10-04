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
                      'running_task.h',
                      'running_task.cc',
                      'select_rows.cc',
                      'select_rows.h',
                      'task.cc',
                      'task.h',
                      'verify.cc',
                      'verify.h',
                      include_directories : [project_include_dir],
                      dependencies : [
                        sqlite_dep,
                        boost_dep,
                      ])

utils = static_library('utils',
   'utils.cc',
   'utils.h',
   include_directories : [project_include_dir],
   dependencies : [
     boost_dep,
   ])

executable('mobile_time_tracker',
           'app_state.cc',
           'app_state.h',
           'csv_exporter.cc',
           'csv_exporter.h',
           'edit_activity_dialog.cc',
           'edit_activity_dialog.h',
           'filtered_activities_dialog.cc',
           'filtered_activities_dialog.h',
           'edit_date_dialog.cc',
           'edit_date_dialog.h',
           'edit_task_dialog.cc',
           'edit_task_dialog.h',
           'export_view.cc',
           'export_view.h',
           'list_model_base.h',
           'main.cc',
           'main_window.cc',
           'main_window.h',
           'recent_activities_model.cc',
           'recent_activities_model.h',
           'activities_list_model_base.cc',
           'activities_list_model_base.h',
           'statistics_view.cc',
           'statistics_view.h',
           'task_list_model_base.cc',
           'task_list_model_base.h',
           'ui_helpers.h',
           'view_with_date_range.cc',
           'view_with_date_range.h',
           install : true,
           gui_app: true,
           include_directories : [project_include_dir],
           link_with: [db_entities, utils],
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
           link_with: [db_entities, utils],
           dependencies : [
               gtest_dep,
               gtest_main_dep,
           ])

test('tests', tests_executable)
