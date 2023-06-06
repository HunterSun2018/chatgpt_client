// std headers
#include <iostream>
#include <string_view>
#include <sstream>
#include <tuple>
#include <regex>
#include <coroutine>
#include <thread>
#include <mutex>
#include <condition_variable>
// boost headers
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/as_tuple.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/spawn.hpp>
// #include <boost/asio/detached.hpp>
// #include <boost/asio/use_awaitable.hpp>

namespace beast = boost::beast;   // from <boost/beast.hpp>
namespace http = beast::http;     // from <boost/beast/http.hpp>
namespace net = boost::asio;      // from <boost/asio.hpp>
namespace ssl = boost::asio::ssl; // from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp; // from <boost/asio/ip/tcp.hpp>
using request = http::request<http::string_body>;

#include "root_certificates.hpp"
#include "http_client.hpp"

using namespace std;

namespace Http
{
    // The SSL context is required, and holds certificates
    ssl::context _ctx{ssl::context::tlsv12_client};
    static net::io_context _ioc;
    static net::io_context::work worker(_ioc); // keep ioc.run() alive until ioc calls function stop
    static std::thread dispatch([]()
        {
            dispatch.detach();

            load_root_certificates(_ctx);

            // Verify the remote server's certificate
            //_ctx.set_verify_mode(ssl::verify_none);
            try
            {
                //clog << "The io dispatch thread is getting startted." << endl;
                _ioc.run();
                //clog << "The io dispatch thread is getting ended." << endl;
            }
            catch (const std::exception& e)
            {
                std::cerr << e.what() << '\n';
            }
        });

    // static std::condition_variable _cv;
    // static std::mutex _cv_m;

    class ClientImp : public Client
    {

        /**
         * @brief parse a url
         *
         * @param url
         * @return tuple<string, string>
         * tuple<protocol, hostname, port, target
         */
        static tuple<string, string, string, string>
            parse_url(const string& url)
        {
            // const std::regex url_regex("(https?)://([^/ :]+):?([^/ ]*)(/?[^ #?]*)\\x3f?([^ #]*)#?([^ ]*)");
            const std::regex url_regex("(https?)://([^/ :]+):?([^/ ]*)((/?[^ #?]*)\\x3f?([^ #]*)#?([^ ]*))");
            std::smatch url_match;

            if (regex_match(url, url_match, url_regex))
            {
                string protocol, hostname, port, target;
                if (url_match.size() >= 3)
                {
                    protocol = url_match[1];
                    hostname = url_match[2];
                    port = url_match[3];
                    target = url_match[4];

                    if (port.empty())
                    {
                        if (protocol == "https")
                            port = "443";
                        else
                            port = "80";
                    }
                }

                return make_tuple<>(protocol, hostname, port, target);
            }
            else
                throw runtime_error("cannot parse url format");
        }

        // Performs an HTTP GET and return the response
        static net::awaitable<void>
            do_session(
                std::string host,
                std::string port,
                const request req,
                beast::http::verb method)
        {
            //  These objects perform our I/O
            auto resolver = net::use_awaitable.as_default_on(tcp::resolver(co_await net::this_coro::executor));
            auto stream = net::use_awaitable.as_default_on(beast::tcp_stream(co_await net::this_coro::executor));

            // Look up the domain name
            auto const results = co_await resolver.async_resolve(host, port);

            // Set the timeout.
            stream.expires_after(std::chrono::seconds(30));

            // Make the connection on the IP address we get from a lookup
            co_await stream.async_connect(results);

            // Set the timeout.
            stream.expires_after(std::chrono::seconds(30));

            // Send the HTTP request to the remote host
            co_await http::async_write(stream, req);

            // This buffer is used for reading and must be persisted
            beast::flat_buffer b;

            // Declare a container to hold the response
            http::response<http::dynamic_body> res;

            // Receive the HTTP response
            co_await http::async_read(stream, b, res);

            // Write the message to standard out
            // std::cout << res << std::endl;

            // Gracefully close the socket
            beast::error_code ec;
            stream.socket().shutdown(tcp::socket::shutdown_both, ec);

            // not_connected happens sometimes
            // so don't bother reporting it.
            //
            if (ec && ec != beast::errc::not_connected)
                throw boost::system::system_error(ec, "shutdown");

            // If we get here then the connection is closed gracefully
            // res.body().;
        }

        // Performs an HTTPS request and return the response
        static net::awaitable<std::string>
            do_session(
                ssl::context& ctx,
                std::string host,
                std::string port,
                request req,
                uint timeout_seconds = 30)
        {
            // These objects perform our I/O
            // They use an executor with a default completion token of use_awaitable
            // This makes our code easy, but will use exceptions as the default error handling,
            // i.e. if the connection drops, we might see an exception.
            // See async_shutdown for error handling with an error_code.
            auto resolver = net::use_awaitable.as_default_on(tcp::resolver(co_await net::this_coro::executor));
            using executor_with_default = net::use_awaitable_t<>::executor_with_default<net::any_io_executor>;
            using tcp_stream = typename beast::tcp_stream::rebind_executor<executor_with_default>::other;

            // We construct the ssl stream from the already rebound tcp_stream.
            beast::ssl_stream<tcp_stream> stream{
                net::use_awaitable.as_default_on(beast::tcp_stream(co_await net::this_coro::executor)),
                    ctx};

            // Set SNI Hostname (many hosts need this to handshake successfully)
            if (!SSL_set_tlsext_host_name(stream.native_handle(), host.c_str()))
                throw boost::system::system_error(static_cast<int>(::ERR_get_error()), net::error::get_ssl_category());

            // Look up the domain name
            auto const results = co_await resolver.async_resolve(host, port);

            // Set the timeout.
            beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(timeout_seconds));

            // Make the connection on the IP address we get from a lookup
            co_await beast::get_lowest_layer(stream).async_connect(results);

            // Set the timeout.
            beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(timeout_seconds));

            // Perform the SSL handshake
            co_await stream.async_handshake(ssl::stream_base::client);

            // Set the timeout.
            beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(timeout_seconds));

            // Send the HTTP request to the remote host
            co_await http::async_write(stream, req);

            // This buffer is used for reading and must be persisted
            beast::flat_buffer b;

            // Declare a container to hold the response
            http::response<http::dynamic_body> res;

            // Receive the HTTP response
            co_await http::async_read(stream, b, res);

            // Set the timeout.
            beast::get_lowest_layer(stream)
                .expires_after(std::chrono::seconds(0));

            // Gracefully close the stream - do not threat every error as an exception!
            // co_await stream.async_shutdown(net::use_awaitable);
            auto [ec] = co_await stream.async_shutdown(net::as_tuple(net::use_awaitable));
            if (ec == net::error::eof)
            {
                // Rationale:
                // http://stackoverflow.com/questions/25587403/boost-asio-ssl-async-shutdown-always-finishes-with-an-error
                ec = {};
            }
            // if (ec)
            //     throw boost::system::system_error(ec, "shutdown");
            // Response result{res.result_int(),
            //                 beast::buffers_to_string(res.body().data())};

            co_return beast::buffers_to_string(res.body().data());
        }

        static void
            http_request(std::string const& host,
                std::string const& port,
                const request& req,
                uint timeout_seconds,
                bool is_ssl,

                function<void(std::exception_ptr e, string&& body)> func)
        {
            if (is_ssl)
            {
                // Verify the remote server's certificate
                _ctx.set_verify_mode(ssl::verify_peer);

                // Launch the asynchronous operation
                net::co_spawn(
                    _ioc,
                    do_session(_ctx, host, port, req, timeout_seconds),
                    // If the awaitable exists with an exception, it gets delivered here as `e`.
                    // This can happen for regular errors, such as connection drops.
                    [=](std::exception_ptr e, string&& body)
                    {
                        func(e, std::move(body));
                    });
            }
            else
            {
                net::co_spawn(_ioc,
                    do_session(host, port, req, beast::http::verb::get),
                    [](std::exception_ptr e)
                    {
                        if (e)
                            try
                        {
                            std::rethrow_exception(e);
                        }
                        catch (std::exception& e)
                        {
                            std::cerr << "Error: " << e.what() << endl;
                        }
                    });
            }
        }

        virtual Task<Response> await_get(std::string_view url, uint timeout_seconds) override
        {
            struct awaitable
            {
                // net::io_context &ioc;
                string _url;
                uint _timeout_seconds;
                Response _response;

                bool await_ready() { return false; }
                void await_suspend(std::coroutine_handle<> h)
                {
                    int version = true ? 11 : 10;

                    auto [protocol, host, port, target] = parse_url(_url);

                    // Set up an HTTP GET request message
                    request req{ http::verb::get, target, version };
                    req.set(http::field::host, host);
                    req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

                    // Launch the asynchronous operation
                    http_request(host, port, req, _timeout_seconds,
                        protocol == "https" ? true : false,
                        [&, h](exception_ptr e, string&& body)
                        {
                            _response.body = std::move(body);
                            h.resume();
                        });
                }

                Response await_resume()
                {
                    return _response;
                }
            };

            co_return co_await awaitable{ url.data(), timeout_seconds };
        }

        virtual Task<Response> await_post(std::string_view url,
            const Request& request,
            uint timeout_seconds) override
        {
            Response response;

            struct awaitable
            {
                // net::io_context &ioc;
                string _url;
                Request _request;
                uint _timeout_seconds;
                Response _response;
                exception_ptr _e;

                bool await_ready() { return false; }
                void await_suspend(std::coroutine_handle<> h)
                {
                    int version = true ? 11 : 10;

                    auto [protocol, host, port, target] = parse_url(_url);

                    // Set up an HTTP GET request message
                    http::request<http::string_body> req{http::verb::post, target, version};
                    req.set(http::field::host, host);
                    req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
                    req.set(http::field::content_length, to_string(_request.body.length()));
                    for (auto iter : _request.header)
                    {
                        req.set(iter.first, iter.second);
                    }
                    req.body() = _request.body;

                    // Launch the asynchronous operation
                    http_request(host, port, req, _timeout_seconds,
                        protocol == "https" ? true : false,
                        [&, h](exception_ptr e, string&& body)
                        {
                            _e = e;
                            _response.body = std::move(body);
                            h.resume();
                        });
                }

                Response await_resume()
                {
                    if (_e)
                        std::rethrow_exception(_e);

                    return _response;
                }
            };

            co_return co_await awaitable{ url.data(), request, timeout_seconds };
        }
    };

    std::shared_ptr<Client>
        Client::create(/* args */)
    {
        // static std::jthread dispatch([]()
        //                              {

        //                              // load_root_certificates(_ctx);

        //                              // Verify the remote server's certificate
        //                              //_ctx.set_verify_mode(ssl::verify_none);
        //                              try
        //                              {
        //                                  clog << "The io dispatch thread is getting startted." << endl;
        //                                  _ioc.run();
        //                                  //clog << "The io dispatch thread is getting ended." << endl;
        //                              }
        //                              catch (const std::exception &e)
        //                              {
        //                                  std::cerr << e.what() << '\n';
        //                              } });
        // dispatch.detach();

        return make_shared<ClientImp>();
    }
}