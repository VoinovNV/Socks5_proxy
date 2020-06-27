
#include <client_uv.hpp>
#include <benchmark/benchmark.h>
#include <thread>
/*
void client_connection(benchmark::State& state)
{
    Proxy::Client_uv a;
    char param[2]={char (int('0')+state.range()),0};
    uv_os_setenv("UV_THREADPOOL_SIZE", param);
    Proxy::req_info req;
    Proxy::req_info_set_default(&req);
    for(auto _:state) a.Start(&req);
    a.run();
}
BENCHMARK(client_connection)->Arg(1)->Arg(2);
*/
void func(){
    Proxy::Client_uv a;
    Proxy::req_info req;
    Proxy::req_info_set_default(&req);
    for(int i=0;i<20;i++) a.Start(&req);
    a.run();
}
int main(){
    std::vector<std::thread> workers;
    size_t extra_workers = 1;
    for(size_t i=0;i<extra_workers;i++){
        std::thread th(func);
        workers.emplace_back(std::move(th));
    }
    func();
    for(auto& t:workers) t.join();

    return 0;

}
