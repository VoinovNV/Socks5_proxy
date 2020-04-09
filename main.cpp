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
//#include <boost/exception/diagnostic_information.hpp>
//#include <boost/log/expressions.hpp>
//#include <boost/log/support/date_time.hpp>
//#include <boost/log/utility/setup/common_attributes.hpp>
//#include <boost/log/utility/setup/console.hpp>

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


int main(int argc,char* argv[]){
    try{
        if(argc!=3){
            BOOST_LOG_TRIVIAL(fatal) << "Usage: " << argv[0] << " <listen-port> <buf-size>\n";
            return 1;
        }
        auto port = from_chars<std::uint16_t>(argv[1]);
        if(!port||!*port){
            std::cerr << "Port must be in [1;65535]\n";
            return 1;
        }
        long long int buf_size= std::atoll(argv[1]);
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

                    //constexpr static std::size_t limit = 20 ;
                    std::string in_buf;
                    std::size_t n;
                    in_buf.resize(257);
                    try{

                        n = co_await stream.async_read_some(
                            boost::asio::buffer(in_buf),
                            boost::asio::use_awaitable);
                        /* TODO:
                         * n = co_await async_read_until(stream,
                            boost::asio::dynamic_string_buffer(in_buf,257),'\0',
                            boost::asio::use_awaitable);
                         */

                    }
                    catch(const boost::system::system_error& e){
                        if(e.code()!=boost::asio::error::operation_aborted&&
                            (e.code()!=boost::asio::error::eof||n))
                            BOOST_LOG_TRIVIAL(error) << "Failed to read Handshake:" << e.what();
                        co_return;
                    }
                    if(in_buf[0]!=0x05) {BOOST_LOG_TRIVIAL(error) <<"Is not SOCKS 5"; co_return;}
                    if(!in_buf.find((char)0)) {BOOST_LOG_TRIVIAL(info) << "unsupported Handshake: 3"; co_return;}
                    std::string out_buf;
                    out_buf.push_back(0x05);
                    out_buf.push_back(0x00);
                    try{
                        co_await async_write(stream,boost::asio::buffer(out_buf),
                                             boost::asio::use_awaitable);
                    }
                    catch(const boost::system::system_error& e){
                        if(e.code()!=boost::asio::error::operation_aborted)
                            BOOST_LOG_TRIVIAL(error) << "Failed to write handshake:" << e.what();
                        co_return;
                    }
                    in_buf.erase(0,257);
                    in_buf.resize(500);
                    try{
                        n = co_await stream.async_read_some(boost::asio::buffer(in_buf), boost::asio::use_awaitable);
                        /* TODO:
                         * n = co_await async_read_until(stream,
                            boost::asio::dynamic_string_buffer(in_buf,257),'\0',
                            boost::asio::use_awaitable);
                         */
                    }
                    catch(const boost::system::system_error& e){
                        if(e.code()!=boost::asio::error::operation_aborted&&
                            (e.code()!=boost::asio::error::eof|| n))
                            BOOST_LOG_TRIVIAL(error) << "Failed to read request: " << e.what();
                        co_return;
                    }
                    /*TODO:
                     * Change ip adressing */
                    if(in_buf[0]!=0x05||in_buf[1]!=0x01||in_buf[2]!=0x00) {BOOST_LOG_TRIVIAL(error) << "Unsupported request: "; co_return;}
                    int a;
                    int b=0;
                    switch(in_buf[3]){
                    case 0x01:{a=4;b=8;break;}
                    case 0x03:{a=5;b=int(in_buf.data()[4]);break;}
                    case 0x04:{a=4;b=5+16; break;}}
                    std::string addr_serv;
                    addr_serv=(in_buf.substr(a,b));
                    std::string port_serv_=std::to_string(ntohs(*(uint16_t*)in_buf.substr(b+a,2).data()));
                    b=0;
                    char con_flag=0x00;
                    boost::asio::ip::tcp::socket socket_to_serv{make_strand(ctx)};
                    boost::asio::ip::tcp::resolver res(ctx);
                    boost::asio::ip::tcp::resolver::query Q(addr_serv,port_serv_);
                    try{
                        // TODO: Async_resolve
                        boost::asio::ip::tcp::resolver::iterator iterator=res.resolve(Q);
                        socket_to_serv.async_connect(iterator->endpoint(),[](const boost::system::error_code& error){if (error) BOOST_LOG_TRIVIAL(error)<<"Connected"<<error.message();});

                    }
                    catch(const boost::system::system_error& e){
                        BOOST_LOG_TRIVIAL(error) << "Connect:" << e.what(); con_flag=0x01;
                    };
                    //socket_to_serv.async_connect(iterator->endpoint(),[](const boost::system::error_code& error, boost::asio::ip::tcp::resolver::iterator i){BOOST_LOG_TRIVIAL(error)<<error.message();});
                    //try {socket_to_serv.async_connect(iterator->endpoint(),[](boost::system::error_code ec){BOOST_LOG_TRIVIAL(error) << "Async Connect:" << ec.message();});}
                    //catch(const boost::system::system_error& e){BOOST_LOG_TRIVIAL(error) << "Async Connect:" << e.what(); con_flag=1;};

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
                    //one shared for both
                    std::shared_ptr<boost::beast::tcp_stream> stream_=std::make_shared<boost::beast::tcp_stream>(std::move(stream)),
                                                              socket_to_serv_=std::make_shared<boost::beast::tcp_stream>(std::move(socket_to_serv));
                    /* TODO: change exception handling:
                     * Exception end of file
                     */

                    co_spawn(ctx,[stream_,socket_to_serv_,buf_size]() -> boost::asio::awaitable<void> {
                        std::string i_b; i_b.resize(buf_size); int n;
                        for(;;){
                            try{
                                n = co_await stream_->async_read_some(boost::asio::buffer(i_b), boost::asio::use_awaitable);
                            }
                            catch(const boost::system::system_error& e){
                                if(e.code()!=boost::asio::error::operation_aborted&&
                                    (e.code()!=boost::asio::error::eof|| n))
                                    BOOST_LOG_TRIVIAL(error) << "Failed to read: client to server: " << e.what();
                                co_return;
                            }
                            try{
                                n = co_await async_write(*socket_to_serv_,boost::asio::buffer(i_b,n),
                                                         boost::asio::use_awaitable);
                            }
                            catch(const boost::system::system_error& e){
                                if(e.code()!=boost::asio::error::operation_aborted&&
                                    (e.code()!=boost::asio::error::eof|| n))
                                    BOOST_LOG_TRIVIAL(error) << "Failed to write from client to server: " << e.what();
                                co_return;
                            }
                        }
                    },boost::asio::detached);
                    for(;;){
                        std::string o_b; int k;
                        o_b.resize(buf_size);
                        try{
                            k = co_await socket_to_serv_->async_read_some(boost::asio::buffer(o_b), boost::asio::use_awaitable);
                        }
                        catch(const boost::system::system_error& e){
                            if(e.code()!=boost::asio::error::operation_aborted&&
                                (e.code()!=boost::asio::error::eof|| k))
                                BOOST_LOG_TRIVIAL(error) << "Failed to read server to client: " << e.what();
                            co_return;
                        }
                        try{
                            k = co_await async_write(*stream_,boost::asio::buffer(o_b,k),
                                                     boost::asio::use_awaitable);
                        }
                        catch(const boost::system::system_error& e){
                            if(e.code()!=boost::asio::error::operation_aborted&&
                                (e.code()!=boost::asio::error::eof|| k))
                                BOOST_LOG_TRIVIAL(error) << "Failed to write server to client: " << e.what();
                            co_return;
                        }
                    }
                },boost::asio::detached);
            }
        },boost::asio::detached);
        std::vector<std::thread> workers;
        size_t extra_workers = std::thread::hardware_concurrency()-1;
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


