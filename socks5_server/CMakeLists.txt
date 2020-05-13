cmake_minimum_required(VERSION 3.12 FATAL_ERROR)

project(socks5_server LANGUAGES CXX)

add_executable(${PROJECT_NAME} main.cpp)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_EXTENSIONS OFF)


#find_package(Boost 1.71.0 REQUIRED COMPONENTS chrono log log_setup system thread)
target_link_libraries(${PROJECT_NAME} PRIVATE
    Boost::chrono Boost::log Boost::system Boost::thread)
get_target_property(_boost_log_lib Boost::log IMPORTED_LOCATION)
if(_boost_log_lib MATCHES "${CMAKE_SHARED_LIBRARY_SUFFIX}$")
    target_link_libraries(${PROJECT_NAME} PRIVATE Boost::dynamic_linking)
endif()
