#include <string>
#include <string_view>
#include <cpprest/filestream.h>
#include <cpprest/http_client.h>
#include "co_helper.hpp"

using namespace std;
using namespace utility;
using namespace web::http;
using namespace web::http::client;
using namespace concurrency::streams;

auto await_http_post(string_t searchTerm)
{
    struct awaitable
    {
        string_t searchTerm;
        string _response;

        bool await_ready() { return false; }
        void await_suspend(std::coroutine_handle<> h)
        {
            // out = std::jthread([h]
            //                    { h.resume(); });

            // Open a stream to the file to write the HTTP response body into.
            // auto fileBuffer = std::make_shared<streambuf<uint8_t>>();

            http_client client(U("http://www.bing.com/"));

            client.request(methods::GET, uri_builder(U("/search")).append_query(U("q"), searchTerm).to_string())
                .then([=, this](http_response response) -> size_t
                      {
            printf("Response status code %u returned.\n", response.status_code());

            _response = response.body(); 
            return 0; });
        }

        void await_resume() {}
    };

    return awaitable{searchTerm};
}

co_helper::Task<void> async_test()
{
    co_await await_http_post("");
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Usage: BingRequest.exe search_term \n");
        return -1;
    }
    const string_t searchTerm = argv[1];

    async_test();

    [](string_view s) -> co_helper::Task<void>
    {
        co_await await_http_post(string_t(s.data()));
    }(argv[1]);

    return 0;
}