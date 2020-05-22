#ifndef CLIENT_UV_HPP
#define CLIENT_UV_HPP
#include <uv.h>
#include <string>
namespace Proxy {

static char dest_adress[255];
static short dest_port;
static short dest_address_len;
static char proxy_adress[255];
static short proxy_port;
static short proxy_address_len;
static char request[500];
static short request_len;

class Client_uv
{
    uv_loop_t *loop;
    //uint16_t port;
   // uv_tcp_t server;
    //std::string address;
    struct sockaddr_in addr;
    static void on_connection(uv_connect_t* con_,int status);
    static void on_close(uv_handle_t* handle);
    static void on_read(uv_stream_t* handle, ssize_t nread, const uv_buf_t* buf);
    static void on_read_addr(uv_stream_t* handle, ssize_t nread, const uv_buf_t* buf);
    static void on_read_req(uv_stream_t* handle, ssize_t nread, const uv_buf_t* buf);
    static void on_write(uv_write_t* req, int status);
    static void on_write_addr(uv_write_t* req, int status);
    static void on_write_req(uv_write_t* req, int status);
    static void on_alloc(uv_handle_t* handle, size_t s_size,uv_buf_t* buf);


public:
    Client_uv(bool flag);
    ~Client_uv();
    void Start();
    int run();


};
}
#endif // CLIENT_UV_HPP
