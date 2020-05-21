#include <iostream>
#include <client_uv.hpp>
int main()
{

    Proxy::Client_uv a(true);
    a.Start();
    return 0;
}
