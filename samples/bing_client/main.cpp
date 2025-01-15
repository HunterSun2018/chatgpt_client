#include <iostream>
#include <co_http_client.hpp>

using namespace std;

Task<void> test();

int main(int argc, char const* argv[])
{
    test().get();

    return 0;
}

Task<void> test()
{
    auto client = co_http::Client::create();

    auto response = co_await client->await_get("https://cn.bing.com/");

    cout << response.body << endl;
}