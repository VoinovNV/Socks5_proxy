#ifndef CLIENT_UV_HPP
#define CLIENT_UV_HPP
#include <uv.h>
#include <string>
namespace Proxy {

struct req_info{
    char proxy_adress[255];
    short proxy_address_len;
    short proxy_port;

    char dest_adress[255];
    short dest_address_len;
    short dest_port;

    char request[500];
    short request_len;
};
void req_info_set_default(req_info* req);
class Client_uv
{
    uv_loop_t *loop;
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
    Client_uv();
    ~Client_uv();
    void Start(req_info* req);
    int run();


};
}
#endif // CLIENT_UV_HPP
