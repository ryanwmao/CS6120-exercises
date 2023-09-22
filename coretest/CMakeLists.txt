add_executable(parse-main ${CMAKE_CURRENT_SOURCE_DIR}/parse_main.cpp)
target_link_libraries(parse-main PRIVATE parse nlohmann_json::nlohmann_json compiler_opts)

add_executable(cfg-main ${CMAKE_CURRENT_SOURCE_DIR}/cfg_main.cpp)
target_link_libraries(cfg-main PRIVATE parse cfg nlohmann_json::nlohmann_json compiler_opts)

add_executable(lives-main ${CMAKE_CURRENT_SOURCE_DIR}/lives_main.cpp)
target_link_libraries(lives-main PRIVATE lives nlohmann_json::nlohmann_json compiler_opts parse)

add_executable(rdefs-main ${CMAKE_CURRENT_SOURCE_DIR}/rdefs_main.cpp)
target_link_libraries(rdefs-main PRIVATE rdefs nlohmann_json::nlohmann_json compiler_opts parse)

add_executable(dom-main ${CMAKE_CURRENT_SOURCE_DIR}/dom_main.cpp)
target_link_libraries(dom-main PRIVATE dom nlohmann_json::nlohmann_json compiler_opts parse)
