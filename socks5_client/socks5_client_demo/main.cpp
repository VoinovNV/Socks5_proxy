
/*

Выдает ошибки вида: ld.lld: error: undefined symbol: uv_default_loop

*/
#include <iostream>
#include <client_uv.hpp>
int main()
{
    Client_uv a(1080,"127.0.0.1");
    a.Start(123,"localhost");
    return 0;
}
