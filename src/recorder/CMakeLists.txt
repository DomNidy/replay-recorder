add_executable(replay_recorder 
    main.cpp
    event_sink.cpp
    user_input_event_source.cpp
    user_window_activity_event_source.cpp
)

target_include_directories(replay_recorder
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../../include/replay-recorder/"
)

