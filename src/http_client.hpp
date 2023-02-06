#pragma once
#include <string_view>
#include <memory>
#include "co_helper.hpp"

namespace http
{
    struct Request
    {
    };

    struct Response
    {
        int status_code = 0;
        std::string body;
    };

    class Client
    {
    private:
        /* data */
    public:
        static std::shared_ptr<Client> create(/* args */);

        virtual ~Client() {}

        // Client(const Client &) = delete;
        Client &operator=(Client &) = delete;
        // Client(Client &&) = delete;
        Client &operator=(Client &&) = delete;

        virtual Task<Response> await_get(std::string_view url) = 0;

        virtual Task<Response> awaitPost(const Request &request) = 0;
    };

    using client_ptr = std::shared_ptr<Client>;

}