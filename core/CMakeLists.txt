add_library(types ${CMAKE_CURRENT_SOURCE_DIR}/types.cpp)
target_link_libraries(types PUBLIC Boost::boost)
target_include_directories(types PUBLIC ${CMAKE_SOURCE_DIR})

add_library(cfg ${CMAKE_CURRENT_SOURCE_DIR}/cfg.cpp)
target_link_libraries(cfg PUBLIC types PRIVATE compiler_opts)
target_include_directories(cfg INTERFACE ${CMAKE_SOURCE_DIR})

add_library(parse ${CMAKE_CURRENT_SOURCE_DIR}/parse.cpp)
target_link_libraries(parse PUBLIC types nlohmann_json::nlohmann_json PRIVATE compiler_opts cfg)
