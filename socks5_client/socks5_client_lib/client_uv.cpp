#include "client_uv.hpp"
#include <iostream>

Client_uv::Client_uv(uint16_t port_,
                     std::string address_)
{
    port=port_;
    address=address_;
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

    }
}
void Client_uv::on_write_req(uv_write_t *req, int status){
    if(status==0){
        uv_read_start(req->handle, on_alloc, on_read_req);
    }
    else{

    }
}

void Client_uv::on_read_addr(uv_stream_t *handle, ssize_t nread, const uv_buf_t *buf){
    if(nread>0){
        if(buf->base[0]==0x05&&buf->base[1]==0x00){
            uv_write_t* req=(uv_write_t *) malloc(sizeof(uv_write_t));
#ifdef SHORT_REQUEST
            SHORT_REQUEST
            int len=SHORT_LEN;
#endif
#ifdef LONG_REQUEST
            LONG_REQUEST
            int len=LONG_LEN;
#endif
            uv_buf_t buf_w=uv_buf_init(p,len);
            uv_write(req,handle,&buf_w,1,on_write_req);
        }
    }
    else{

    }
}

void Client_uv::on_write_addr(uv_write_t* req, int status){
    if(status==0){
        uv_read_start(req->handle, on_alloc, on_read_addr);
    }
    else{

    }
}
void Client_uv::on_read(uv_stream_t* handle, ssize_t nread, const uv_buf_t* buf){
    if(nread>0){
        if(nread==2&&buf->base[0]==0x05&&buf->base[1]==0x00){
            uv_write_t* req=(uv_write_t *) malloc(sizeof(uv_write_t));;
            std::string a="\5\1\0\3";
            a.insert(a.size(), std::string_view(DEST_ADDR));
            short p=DEST_PORT;
            char port[2]={((char*)&(p))[0],((char*)&(p))[1]};
            a.insert(a.size(),std::string_view(port));
            char p_[500];
            std::memcpy(p_,a.data(),a.size());
            uv_buf_t buf_w=uv_buf_init(p_,a.size());
            uv_write(req,handle,&buf_w,1,on_write_addr);
        }
        else{

        }
    }
    else{
        /*if (err=uv_eof) uv_close(uv_handle)
         * else signal_error
         *
         *
         */
    }

    free(buf->base);
}

void Client_uv::on_alloc(uv_handle_t* handle, size_t s_size,uv_buf_t* buf){
    buf->base=(char*)malloc(s_size);
    buf->len=s_size;
}
void Client_uv::on_write(uv_write_t* req, int status){
    uv_read_start(req->handle, on_alloc, on_read);
}
void Client_uv::on_connection(uv_connect_t* con_, int status){
    if(status==0){
        /*success*/
        uv_write_t* req=(uv_write_t *) malloc(sizeof(uv_write_t));;
        char a[3]={'\5','\1','\0'};
        uv_buf_t buf_w=uv_buf_init(a,3);
        uv_write(req,con_->handle,&buf_w,1,on_write);
    }
    else{

    }
    //uv_close((uv_handle_t*)handle,on_close);
}
void Client_uv::Start(uint16_t dest_port_,
                      std::string dest_address_){

    uv_tcp_t* socket = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
    uv_tcp_init(loop, socket);

    uv_connect_t* connect = (uv_connect_t*)malloc(sizeof(uv_connect_t));
    struct sockaddr_in dest;
    uv_ip4_addr(dest_address_.data(), dest_port_, &dest);
    uv_tcp_connect(connect, socket, (const struct sockaddr*)&dest, on_connection);

    int r = uv_run(loop,UV_RUN_DEFAULT);
}
