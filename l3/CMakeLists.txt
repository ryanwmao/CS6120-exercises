###############
# EXECUTABLES #
###############

add_executable(tdce ${CMAKE_CURRENT_SOURCE_DIR}/tdce.cpp)
target_link_libraries(tdce PRIVATE nlohmann_json::nlohmann_json compiler_opts)