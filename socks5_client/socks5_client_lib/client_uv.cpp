#include "client_uv.hpp"
#include <iostream>
#include <boost/log/trivial.hpp>
namespace Proxy{
void req_info_set_default(req_info* req){
    req->dest_port=80;
    req->dest_address_len=4;
    std::int8_t a[4]={127,0,0,1};
    std::memcpy(req->dest_adress,a,req->dest_address_len);
    req->proxy_port=8000;
    req->proxy_address_len=9;
    std::memcpy(req->proxy_adress,"localhost",req->proxy_address_len);
    req->request_len=55;
    std::memcpy(req->request,"GET / HTTP/1.1\r\nHost: local host\r\nConnection: close\r\n\r\n",req->request_len);

}
Client_uv::Client_uv()
{

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
        //BOOST_LOG_TRIVIAL(info)<<buf->base;
        uv_close((uv_handle_t *)handle,on_close);
        //uv_read_start(handle, on_alloc, on_read_req);

    }
    else{
        if(nread!=UV_EOF) BOOST_LOG_TRIVIAL(error)<<"Can't read server answer";
        uv_close((uv_handle_t *)handle,on_close);
    }
}
void Client_uv::on_write_req(uv_write_t *req, int status){
    if(status==0){
        uv_read_start(req->handle, on_alloc, on_read_req); //Read until EOF or error
    }
    else{
        BOOST_LOG_TRIVIAL(error)<<"Can't write request to local server";
        uv_close((uv_handle_t *)req->handle,on_close);
    }
}

void Client_uv::on_read_addr(uv_stream_t *handle, ssize_t nread, const uv_buf_t *buf){
    if(nread>0){
        //if(nread==10){
            if(buf->base[0]==0x05&&buf->base[1]==0x00){
                uv_write_t* req=(uv_write_t *) malloc(sizeof(uv_write_t));
                req_info* req_i=(req_info*)(handle->data);
                uv_buf_t buf_w=uv_buf_init(req_i->request,req_i->request_len);
                uv_write(req,handle,&buf_w,1,on_write_req);
            }
        //}
    }
    else{
        if(nread!=UV_EOF) BOOST_LOG_TRIVIAL(error)<<"Can't read proxy answer"<<uv_strerror(nread);
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
        if(/*nread==2&&*/buf->base[0]==0x05&&buf->base[1]==0x00){
            uv_write_t* req=(uv_write_t *) malloc(sizeof(uv_write_t));;
            req_info* req_i=(req_info*)(handle->data);
            char a[10]={0x05,0x01,0x00,0x01};
            std::memcpy(a+4,req_i->dest_adress,req_i->dest_address_len);
            auto r=htons(req_i->dest_port);
            std::memcpy(a+4+req_i->dest_address_len,&r,2);

            uv_buf_t buf_w=uv_buf_init(a,10);

            uv_write(req,handle,&buf_w,1,on_write_addr);
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
    if(status==0)
    uv_read_start(req->handle, on_alloc, on_read);
    else {
        BOOST_LOG_TRIVIAL(error)<<"Can't write handshake";
        uv_close((uv_handle_t*)req->handle,on_close);

    }
}
void Client_uv::on_connection(uv_connect_t* con_, int status){
    if(status==0){
        /*success*/
        uv_write_t* req=(uv_write_t *) malloc(sizeof(uv_write_t));
        char a[3]={0x05,0x01,0x00};
        uv_buf_t buf_w=uv_buf_init(a,3);
        con_->handle->data=con_->data;
        uv_write(req,con_->handle,&buf_w,1,on_write);
    }
    else{

        BOOST_LOG_TRIVIAL(error)<<"Can't connect to proxy";
        uv_close((uv_handle_t*)con_->handle,on_close);
    }

}
void Client_uv::Start(req_info* req_i){
    uv_tcp_t* socket = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
    uv_tcp_init(loop, socket);

    uv_connect_t* connect = (uv_connect_t*)malloc(sizeof(uv_connect_t));
    struct sockaddr_in dest;

    uv_ip4_addr(req_i->proxy_adress, req_i->proxy_port, &dest);

    connect->data=req_i;

    int d =uv_tcp_connect(connect, socket, (const struct sockaddr*)&dest, on_connection);
}
int Client_uv::run (){
    return uv_run(loop,UV_RUN_DEFAULT);
}

}
