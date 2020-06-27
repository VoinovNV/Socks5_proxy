cmake_minimum_required(VERSION 3.8 FATAL_ERROR)

set(NAMESPACE socks5_client)
set(COMPONENT demo)
project(${NAMESPACE}_${COMPONENT}
    VERSION 0.0.0
    LANGUAGES CXX
)

add_executable(${PROJECT_NAME}
    main.cpp)

add_executable(${NAMESPACE}::${COMPONENT} ALIAS ${PROJECT_NAME})
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)

find_package(socks5_client_lib 0.0.0 REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE socks5_client::lib
    benchmark::benchmark_main Boost::chrono Boost::log Boost::system Boost::thread)
get_target_property(_boost_log_lib Boost::log IMPORTED_LOCATION)
if(_boost_log_lib MATCHES "${CMAKE_SHARED_LIBRARY_SUFFIX}$")
    target_link_libraries(${PROJECT_NAME} PRIVATE Boost::dynamic_linking)
endif()


