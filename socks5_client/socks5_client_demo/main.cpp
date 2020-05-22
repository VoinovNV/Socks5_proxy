
#include <client_uv.hpp>
#include <benchmark/benchmark.h>


void client_connection(benchmark::State& state)
{
    Proxy::Client_uv a(true);
    char param[2]={char (int('0')+state.range()),0};
    uv_os_setenv("UV_THREADPOOL_SIZE", param);
    for(auto _:state) a.Start();
    a.run();
}
BENCHMARK(client_connection)->Arg(1)->Arg(2);
