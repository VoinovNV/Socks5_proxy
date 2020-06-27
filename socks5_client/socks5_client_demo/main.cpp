
#include <client_uv.hpp>
#include <benchmark/benchmark.h>
#include <thread>
#include <boost/log/trivial.hpp>
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
    auto n=std::chrono::high_resolution_clock::now();
    for(size_t i=0;i<extra_workers;i++){
        std::thread th(func);
        workers.emplace_back(std::move(th));
    }
    func();
    for(auto& t:workers) t.join();
    auto n1=std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> d=n1-n;
    BOOST_LOG_TRIVIAL(info)<<d.count();
    return 0;

}
