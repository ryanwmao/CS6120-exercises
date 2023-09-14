add_executable(parse-main ${CMAKE_CURRENT_SOURCE_DIR}/parse_main.cpp)
target_link_libraries(parse-main PRIVATE parse nlohmann_json::nlohmann_json compiler_opts)

add_executable(cfg-main ${CMAKE_CURRENT_SOURCE_DIR}/cfg_main.cpp)
target_link_libraries(cfg-main PRIVATE parse cfg nlohmann_json::nlohmann_json compiler_opts)
