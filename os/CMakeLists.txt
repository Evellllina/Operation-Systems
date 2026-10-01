if(WIN32)
    set(OS_SOURCES process.cpp process_win.cpp)
else()
    set(OS_SOURCES process.cpp process_posix.cpp)
endif()

add_library(os STATIC ${OS_SOURCES})
target_include_directories(os PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(os PUBLIC common)