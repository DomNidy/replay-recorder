# --- Create library containing the recording functionality --- #
add_library(
  recorder_lib STATIC
  event_sink.cpp
  user_input_event_source.cpp
  user_window_activity_event_source.cpp
  screenshot_event_source.cpp
  screenshot_timing_strategy.cpp
  screenshot_serialization_strategy.cpp
  windows_hook_manager.cpp)

target_include_directories(recorder_lib
                           PUBLIC "${PROJECT_SOURCE_DIR}/include/recorder")
target_link_libraries(
  recorder_lib
  PRIVATE project_options
  PUBLIC replay_utils)

# --- Create executable target for CLI functionality --- #
add_executable(replay_recorder main.cpp)

target_link_libraries(replay_recorder PRIVATE project_options recorder_lib)
