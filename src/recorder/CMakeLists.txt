# --- Create library containing the recording functionality --- #
add_library(recorder_lib STATIC
    event_sink.cpp
    user_input_event_source.cpp
    user_window_activity_event_source.cpp
)

target_include_directories(recorder_lib
    PUBLIC "${PROJECT_SOURCE_DIR}/include/replay/recorder"
)
target_link_libraries(recorder_lib PUBLIC spdlog::spdlog_header_only)


# --- Create executable target for CLI functionality --- #
add_executable(replay_recorder
    main.cpp
)

target_link_libraries(replay_recorder
    PRIVATE recorder_lib spdlog::spdlog_header_only
)

