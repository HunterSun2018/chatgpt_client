#pragma once

#include <string_view>
#include <map>
#include <memory>
#include "co_helper.hpp"

namespace Http
{
    struct Request
    {
        std::map<std::string, std::string> header;
        std::string body;
    };

    struct Response
    {
        uint status_code = 0;
        std::string body;
    };

    class Client
    {
    public:
        static std::shared_ptr<Client> create(/* args */);

        virtual ~Client() {}

        virtual Task<Response> await_get(std::string_view url, uint timeout_seconds = 30) = 0;

        virtual Task<Response> await_post(std::string_view url, const Request& request, uint timeout_seconds = 30) = 0;
    };

    using client_ptr = std::shared_ptr<Client>;

}