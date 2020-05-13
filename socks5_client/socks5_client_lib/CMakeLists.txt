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
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
find_package(LibUV REQUIRED)
if (LIBUV_FOUND)
    include_directories(SYSTEM ${LIBUV_INCLUDE_DIRS})
endif (LIBUV_FOUND)
