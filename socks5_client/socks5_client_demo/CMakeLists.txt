cmake_minimum_required(VERSION 3.8 FATAL_ERROR)

set(NAMESPACE socks5_client)
set(COMPONENT demo)
project(${NAMESPACE}_${COMPONENT}
    VERSION 0.0.0
    LANGUAGES C
)

add_executable(${PROJECT_NAME}
    main.cpp)

add_executable(${NAMESPACE}::${COMPONENT} ALIAS ${PROJECT_NAME})

find_package(socks5_client_lib 0.0.0 REQUIRED)
target_link_libraries(${PROJECT_NAME} socks5_client::lib)

 
