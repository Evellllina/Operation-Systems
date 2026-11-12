add_library(common STATIC errors.cpp)
target_include_directories(common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)