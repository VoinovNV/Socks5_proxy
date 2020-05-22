cmake_minimum_required(VERSION 3.16)

project(socks5_proxy
    VERSION 0.0.0
    LANGUAGES CXX
)

find_package(PkgConfig REQUIRED)
find_package(Boost 1.71.0 REQUIRED COMPONENTS
    chrono container coroutine log system thread)
find_package(benchmark REQUIRED)
add_subdirectory(socks5_server)
add_subdirectory(socks5_client)
