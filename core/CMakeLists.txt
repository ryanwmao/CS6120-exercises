add_library(types INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/types.hpp)
target_include_directories(types INTERFACE ${CMAKE_SOURCE_DIR})

add_library(parse ${CMAKE_CURRENT_SOURCE_DIR}/parse.cpp)
target_link_libraries(parse PUBLIC types nlohmann_json::nlohmann_json PRIVATE compiler_opts)
