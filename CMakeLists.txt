cmake_minimum_required(VERSION 3.16)

project(socks5_proxy
    VERSION 0.0.0
    LANGUAGES CXX
)

find_package(PkgConfig REQUIRED)
# ASIO itself is a header-only library, so it has no corresponding
# component, but depends on non-header-only libraries chrono, system and thread.
# use_awaitable_t::as_default_on is in 1.72, but buggy, fix in 1.73.
find_package(Boost 1.71.0 REQUIRED COMPONENTS
    chrono container coroutine log system thread)
#find_package(benchmark REQUIRED)

add_subdirectory(socks5_server)
add_subdirectory(socks5_client)
