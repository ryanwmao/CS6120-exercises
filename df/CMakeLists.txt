add_library(lives ${CMAKE_CURRENT_SOURCE_DIR}/lives.cpp)
target_link_libraries(lives PUBLIC types PRIVATE compiler_opts)

add_library(rdefs ${CMAKE_CURRENT_SOURCE_DIR}/rdefs.cpp)
target_link_libraries(rdefs PUBLIC types PRIVATE compiler_opts)
