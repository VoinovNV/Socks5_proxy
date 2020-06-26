#include <boost/asio/awaitable.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ip/v6_only.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/asio/write.hpp>
#include <boost/beast/core/tcp_stream.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/bind.hpp>
#include <charconv>
#include <cstdint>
#include <iostream>
#include <memory>
//#include <mutex>
#include <optional>
#include <stdexcept>
#include <sstream>
#include <thread>
#include <type_traits>
#include <boost/log/trivial.hpp>
#include <boost/regex.hpp>
template<typename T>
std::optional<T> from_chars(std::string_view sv) noexcept
{
    T out;
    auto end = sv.data()+sv.size();
    auto res = std::from_chars(sv.data(),end,out);
    if(res.ec==std::errc{}&&res.ptr==end)
        return out;
    return {};
}

template<typename F>
auto at_scope_exit(F&& f)
{
    using f_t = std::remove_cvref_t<F>;
    static_assert(std::is_nothrow_destructible_v<f_t>&&
                  std::is_nothrow_invocable_v<f_t>);
    struct ase_t
    {
        F f;

        ase_t(F&& f)
            : f(std::forward<F>(f))
        {}

        ase_t(const ase_t&) = default;
        ase_t(ase_t&&) = delete;
        ase_t operator=(const ase_t&) = delete;
        ase_t operator=(ase_t&&) = delete;

        ~ase_t()
        {
            std::forward<F>(f)();
        }
    };
    return ase_t{std::forward<F>(f)};
}

auto my_send(boost::asio::io_context& ctx,std::shared_ptr<boost::beast::tcp_stream> dest,std::shared_ptr<boost::beast::tcp_stream> src, std::uint32_t buf_size,std::string_view name)
{co_spawn(ctx,[name,dest,src,buf_size]() -> boost::asio::awaitable<void> {
            std::string i_b; i_b.resize(buf_size);
            int n;
            for(;;){
                try{
                    n = co_await dest->async_read_some(boost::asio::buffer(i_b), boost::asio::use_awaitable);
                }
                catch(const boost::system::system_error& e){
                    if(e.code()!=boost::asio::error::operation_aborted&&
                        (e.code()!=boost::asio::error::eof|| n))
                        BOOST_LOG_TRIVIAL(error) << "Failed to read: " <<name<< e.what();
                    co_return;
                }
                try{
                    n = co_await async_write(*src,boost::asio::buffer(i_b,n),
                                             boost::asio::use_awaitable);
                }
                catch(const boost::system::system_error& e){
                    if(e.code()!=boost::asio::error::operation_aborted&&
                        (e.code()!=boost::asio::error::eof|| n))
                        BOOST_LOG_TRIVIAL(error) << "Failed to write: "<<name << e.what();
                    co_return;
                }
            }
        },boost::asio::detached);}
int main(int argc,char* argv[]){
    try{
        if(argc!=4){
            BOOST_LOG_TRIVIAL(fatal) << "Usage: " << argv[0] << " <listen-port> <buf-size> <thread-num>\n";
            return 1;
        }
        auto port = from_chars<std::uint16_t>(argv[1]);
        if(!port||!*port){
            std::cerr << "Port must be in [1;65535]\n";
            return 1;
        }
        auto buf_size= from_chars<std::uint32_t>(argv[2]);
        if(!port||!*port){
            std::cerr << "Buf_size error\n";
            return 1;
        }
        auto thr_num= from_chars<std::size_t>(argv[3]);
        if(!port||!*port){
            std::cerr << "Thread number error\n";
            return 1;
        }

        boost::asio::io_context ctx;
        boost::asio::signal_set stop_signals{ctx,SIGINT,SIGTERM};
        stop_signals.async_wait([&](boost::system::error_code ec,int /*signal*/){
            if(ec)
                return;
            BOOST_LOG_TRIVIAL(info) << "Terminating in response to signal.";
            ctx.stop();
        });

        co_spawn(ctx,[&ctx,port=*port,buf_size]() -> boost::asio::awaitable<void> {
            boost::asio::ip::tcp::acceptor acceptor{ctx};
            boost::asio::ip::tcp::endpoint ep{boost::asio::ip::tcp::v6(),port};
            acceptor.open(ep.protocol());
            acceptor.set_option(boost::asio::ip::v6_only{false});
            acceptor.bind(ep);
            acceptor.listen();
            BOOST_LOG_TRIVIAL(info) << "Listening on port " << port << "...";
            for(;;){
                boost::asio::ip::tcp::socket socket{make_strand(acceptor.get_executor())};
                try{
                    co_await acceptor.async_accept(socket,boost::asio::use_awaitable);
                }
                catch(const boost::system::system_error& e){
                    if(e.code()!=boost::asio::error::operation_aborted)
                        BOOST_LOG_TRIVIAL(error) << "Failed to accept connection: " << e.what();
                    co_return;
                }
                co_spawn(socket.get_executor(),
                      [&ctx,stream=boost::beast::tcp_stream{std::move(socket)},buf_size]
                          () mutable -> boost::asio::awaitable<void> {

                    std::string in_buf;
                    std::size_t n;
                    in_buf.resize(257);
                    //Read 2 bytes of handshake
                    try{
                        n = co_await async_read(stream, boost::asio::buffer(in_buf),
                                boost::asio::transfer_exactly(2), boost::asio::use_awaitable);

                    }
                    catch(const boost::system::system_error& e){
                        if(e.code()!=boost::asio::error::operation_aborted&&
                            (e.code()!=boost::asio::error::eof||n))
                            BOOST_LOG_TRIVIAL(error) << "Failed to read Handshake:" << e.what();
                        co_return;
                    }

                    if(in_buf[0]!='\5'){BOOST_LOG_TRIVIAL(error) <<"Is not SOCKS 5"; co_return;}
                    unsigned i=in_buf[1];
                    //read all possible auth bytes
                    try{
                        n = co_await async_read(stream, boost::asio::buffer(in_buf),
                                boost::asio::transfer_exactly(i), boost::asio::use_awaitable);

                    }
                    catch(const boost::system::system_error& e){
                        if(e.code()!=boost::asio::error::operation_aborted&&
                            (e.code()!=boost::asio::error::eof||n))
                            BOOST_LOG_TRIVIAL(error) << "Failed to read Handshake:" << e.what();
                        co_return;
                    }
                    if(in_buf.size()<i){ BOOST_LOG_TRIVIAL(error) <<"Is not SOCKS 5"; co_return;}
                    std::byte ch_auth[2]={std::byte{5},std::byte{0}};
                    try{
                        co_await async_write(stream,boost::asio::buffer(ch_auth,2),
                                             boost::asio::use_awaitable);
                    }
                    catch(const boost::system::system_error& e){
                        if(e.code()!=boost::asio::error::operation_aborted)
                            BOOST_LOG_TRIVIAL(error) << "Failed to write handshake:" << e.what();
                        co_return;
                    }
                    in_buf.erase(0,257);
                    in_buf.resize(500);
                    i=4;
                    try{
                        n = co_await async_read(stream,boost::asio::buffer(in_buf), boost::asio::transfer_exactly(i),boost::asio::use_awaitable);
                    }
                    catch(const boost::system::system_error& e){
                        if(e.code()!=boost::asio::error::operation_aborted&&
                            (e.code()!=boost::asio::error::eof|| n))
                            BOOST_LOG_TRIVIAL(error) << "Failed to read request: " << e.what();
                        co_return;
                    }
                    if(in_buf[0]!=0x05||in_buf[1]!=0x01||in_buf[2]!=0x00) {BOOST_LOG_TRIVIAL(error) << "Unsupported request"; co_return;}
                    int type_adr=in_buf[3];
                    switch(type_adr){
                    case 0x01: {i=6;break;}
                    case 0x03: {i=1;break;}
                    default: co_return;
                    }

                    try{
                        n = co_await async_read(stream,boost::asio::buffer(in_buf), boost::asio::transfer_exactly(i),boost::asio::use_awaitable);
                    }
                    catch(const boost::system::system_error& e){
                        if(e.code()!=boost::asio::error::operation_aborted&&
                            (e.code()!=boost::asio::error::eof|| n))
                            BOOST_LOG_TRIVIAL(error) << "IP4 wrong addr: " << e.what();
                        co_return;
                    }
                    int a,b;
                    std::string addr_serv;
                    std::string port_serv_;
                    if(type_adr==0x01){
                        a=0;b=4;
                        struct in_addr addr;
                        addr.s_addr = *(std::uint32_t*)in_buf.data();
                        addr_serv=inet_ntoa(addr);
                        addr_serv.push_back('\0');

                        port_serv_=std::to_string(ntohs(*(uint16_t*)in_buf.substr(b+a,2).data()));
                        //port_serv_=in_buf.substr(b+a,2);

                    }
                    else{
                        a=0;b=in_buf[0];
                        try{
                            n = co_await async_read(stream,boost::asio::buffer(in_buf), boost::asio::transfer_exactly(in_buf[0]+2),boost::asio::use_awaitable);
                        }
                        catch(const boost::system::system_error& e){ if(e.code()!=boost::asio::error::operation_aborted&&
                                (e.code()!=boost::asio::error::eof|| n))
                                BOOST_LOG_TRIVIAL(error) << "DNS: wrong addr " << e.what();
                            co_return;
                        }

                        addr_serv=(in_buf.substr(a,b));
                        port_serv_=std::to_string(ntohs(*(uint16_t*)in_buf.substr(b+a,2).data()));
                        BOOST_LOG_TRIVIAL(error)<<int(in_buf.substr(b+a,2)[0])<<' '<<int(in_buf.substr(b+a,2)[1])<<"as";
                    }

                    b=0;
                    char con_flag=0x00;
                    boost::asio::ip::tcp::socket socket_to_serv{make_strand(ctx)};
                    boost::asio::ip::tcp::resolver res(ctx);
                    boost::asio::ip::tcp::resolver::query Q(addr_serv,port_serv_);
                    BOOST_LOG_TRIVIAL(info)<<"Try connect to: " << addr_serv<<' '<<port_serv_;

                    try{
                        boost::asio::ip::tcp::resolver::iterator iterator=co_await res.async_resolve(Q,boost::asio::use_awaitable);//res.resolve(Q);
                        co_await socket_to_serv.async_connect(iterator->endpoint(),boost::asio::use_awaitable/*[](const boost::system::error_code& error){
                            if (error)BOOST_LOG_TRIVIAL(error)<<"Connection: "<<error.message();}*/);
                    }
                    catch(const boost::system::system_error& e){
                        BOOST_LOG_TRIVIAL(error) << "Connection: " << e.what(); con_flag=0x01;
                        co_return;
                    };
                    std::string answer; answer.resize(10);
                    answer[0]=(0x05);
                    answer[1]=(con_flag);
                    answer[2]=(0x00);
                    answer[3]=(0x01);
                    uint32_t an_ad=htonl(socket_to_serv.local_endpoint().address().to_v4().to_uint());
                    uint16_t an_pt=htons(socket_to_serv.local_endpoint().port());
                    std::memcpy(&answer[4],&an_ad,4);
                    std::memcpy(&answer[8],&an_pt,2);

                    try{
                        co_await async_write(stream,boost::asio::buffer(answer),
                                             boost::asio::use_awaitable);
                    }
                    catch(const boost::system::system_error& e){
                        if(e.code()!=boost::asio::error::operation_aborted)
                            BOOST_LOG_TRIVIAL(error) << "Failed to write answer to request:" << e.what();
                        co_return;
                    }
                    std::shared_ptr<boost::beast::tcp_stream> stream_=std::make_shared<boost::beast::tcp_stream>(std::move(stream)),
                                                              socket_to_serv_=std::make_shared<boost::beast::tcp_stream>(std::move(socket_to_serv));

                    my_send(ctx,socket_to_serv_,stream_,buf_size.value(),"from server");
                    my_send(ctx,stream_,socket_to_serv_,buf_size.value(),"from client");
                },boost::asio::detached);
            }
        },boost::asio::detached);
        std::vector<std::thread> workers;
        size_t extra_workers = thr_num.value();
        workers.reserve(extra_workers);
        auto ase = at_scope_exit([&]() noexcept {
            for(auto& t:workers)
                t.join();
        });
        for(size_t i=0;i<extra_workers;++i)
            workers.emplace_back([&]{
                ctx.run();
            });
        ctx.run();

    }
    catch(...){
        BOOST_LOG_TRIVIAL(fatal) <<"Uncaught exception in main thread: "<< boost::current_exception_diagnostic_information();
        return 1;
    }
}


