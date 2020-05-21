cmake_minimum_required(VERSION 3.8 FATAL_ERROR)

set(NAMESPACE socks5_client)
set(COMPONENT lib)
project(${NAMESPACE}_${COMPONENT}
    VERSION 0.0.0
    LANGUAGES CXX
)


add_library(${PROJECT_NAME}
    client_uv.hpp
    client_uv.cpp
)

add_library(${NAMESPACE}::${COMPONENT} ALIAS ${PROJECT_NAME})
target_include_directories(${PROJECT_NAME} PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>")
find_package(PkgConfig REQUIRED)
Pkg_check_modules(uv REQUIRED IMPORTED_TARGET libuv)
target_link_libraries(${PROJECT_NAME} PRIVATE PkgConfig::uv
    Boost::chrono Boost::log Boost::system Boost::thread)
get_target_property(_boost_log_lib Boost::log IMPORTED_LOCATION)
if(_boost_log_lib MATCHES "${CMAKE_SHARED_LIBRARY_SUFFIX}$")
    target_link_libraries(${PROJECT_NAME} PRIVATE Boost::dynamic_linking)
endif()
