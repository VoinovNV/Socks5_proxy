#include "client_uv.hpp"
#include <iostream>
#include <boost/log/trivial.hpp>
namespace Proxy{
Client_uv::Client_uv(bool flag)
{
    Proxy::dest_port=1080;
    Proxy::dest_address_len=9;
    std::memcpy(Proxy::dest_adress,"localhost",Proxy::dest_address_len);
    Proxy::proxy_port=1080;
    Proxy::proxy_address_len=9;
    std::memcpy(Proxy::proxy_adress,"localhost",Proxy::proxy_address_len);
    if (flag){
        Proxy::request_len=2;
        std::memcpy(Proxy::request,"\1\1",Proxy::request_len);
    }
    else{
        Proxy::request_len=5;
        std::memcpy(Proxy::request,"Hello",Proxy::request_len);
    }
    loop = uv_default_loop();
    uv_loop_init(loop);


}
Client_uv::~Client_uv(){

}

void Client_uv::on_close(uv_handle_t *handle){
    free(handle);
}
void Client_uv::on_read_req(uv_stream_t *handle, ssize_t nread, const uv_buf_t *buf){
    if (nread>0){
        uv_close((uv_handle_t *)handle,on_close);
    }
    else{
        BOOST_LOG_TRIVIAL(error)<<"Can't read server answer";
        uv_close((uv_handle_t *)handle,on_close);
    }
}
void Client_uv::on_write_req(uv_write_t *req, int status){
    if(status==0){
        uv_read_start(req->handle, on_alloc, on_read_req);
    }
    else{
        BOOST_LOG_TRIVIAL(error)<<"Can't write request to local server";
        uv_close((uv_handle_t *)req->handle,on_close);
    }
}

void Client_uv::on_read_addr(uv_stream_t *handle, ssize_t nread, const uv_buf_t *buf){
    if(nread>0){
        if(buf->base[0]==0x05&&buf->base[1]==0x00){
            uv_write_t* req=(uv_write_t *) malloc(sizeof(uv_write_t));

            uv_buf_t buf_w=uv_buf_init(Proxy::request,Proxy::request_len);
            uv_write(req,handle,&buf_w,1,on_write_req);
        }
        else{
            BOOST_LOG_TRIVIAL(error)<<"Something wrong: read proxy answer";
            uv_close((uv_handle_t *)handle,on_close);
        }
    }
    else{
        BOOST_LOG_TRIVIAL(error)<<"Can't read proxy answer";
        uv_close((uv_handle_t *)handle,on_close);
    }
}

void Client_uv::on_write_addr(uv_write_t* req, int status){
    if(status==0){
        uv_read_start(req->handle, on_alloc, on_read_addr);
    }
    else{
        BOOST_LOG_TRIVIAL(error)<<"Can't write request to proxy";
        uv_close((uv_handle_t *)req->handle,on_close);
    }
}
void Client_uv::on_read(uv_stream_t* handle, ssize_t nread, const uv_buf_t* buf){
    if(nread>0){
        if(nread==2&&buf->base[0]==0x05&&buf->base[1]==0x00){
            uv_write_t* req=(uv_write_t *) malloc(sizeof(uv_write_t));;

            char a[500]={0x05,0x01,0x00,0x03,0x09};
            std::memcpy(a+5,Proxy::dest_adress,Proxy::dest_address_len);
            std::memcpy(a+5+Proxy::proxy_address_len,&Proxy::dest_port,2);
            uv_buf_t buf_w=uv_buf_init(a,500);
            uv_write(req,handle,&buf_w,1,on_write_addr);
        }
        else{
            BOOST_LOG_TRIVIAL(error)<<"Something wrong on handshake";
            uv_close((uv_handle_t*)handle,on_close);
        }
    }
    else{
        BOOST_LOG_TRIVIAL(error)<<"Can't read handshake";
        uv_close((uv_handle_t*)handle,on_close);
    }

    free(buf->base);
}

void Client_uv::on_alloc(uv_handle_t* handle, size_t s_size,uv_buf_t* buf){
    buf->base=(char*)malloc(s_size);
    buf->len=s_size;
}
void Client_uv::on_write(uv_write_t* req, int status){
    if(status==0) uv_read_start(req->handle, on_alloc, on_read);
    else {
        BOOST_LOG_TRIVIAL(error)<<"Can't write handshake";
        uv_close((uv_handle_t*)req->handle,on_close);
    }
}
void Client_uv::on_connection(uv_connect_t* con_, int status){
    if(status==0){
        /*success*/
        uv_write_t* req=(uv_write_t *) malloc(sizeof(uv_write_t));;
        char a[3]={0x05,0x01,0x00};
        uv_buf_t buf_w=uv_buf_init(a,3);
        uv_write(req,con_->handle,&buf_w,1,on_write);
    }
    else{

        BOOST_LOG_TRIVIAL(error)<<"Can't connect to proxy";
        uv_close((uv_handle_t*)con_->handle,on_close);
    }

}
void Client_uv::Start(){
    uv_tcp_t* socket = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
    uv_tcp_init(loop, socket);

    uv_connect_t* connect = (uv_connect_t*)malloc(sizeof(uv_connect_t));
    struct sockaddr_in dest;
    uv_ip4_addr(Proxy::proxy_adress, Proxy::proxy_port, &dest);

    int d =uv_tcp_connect(connect, socket, (const struct sockaddr*)&dest, on_connection);
}
int Client_uv::run (){
    return uv_run(loop,UV_RUN_DEFAULT);
}

}
