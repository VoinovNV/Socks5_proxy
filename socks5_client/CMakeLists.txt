cmake_minimum_required(VERSION 3.12 FATAL_ERROR)

project(socks5_client
    VERSION 0.0.0
    LANGUAGES CXX
)
set(SUBPROJECTS
    socks5_client_lib
    socks5_client_demo
)
macro(find_package)
    if(NOT "${ARGV0}" IN_LIST SUBPROJECTS)
        _find_package(${ARGV})
    endif()
endmacro()

foreach(subdir IN LISTS SUBPROJECTS)
    add_subdirectory(${subdir})
endforeach()

