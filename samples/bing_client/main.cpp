#include <iostream>
#include <http_client.hpp>

Task<void> test();
using namespace std;

int main(int argc, char const* argv[])
{
    test().get();

    return 0;
}

Task<void> test()
{
    auto client = Http::Client::create();

    auto response = co_await client->await_get("https://cn.bing.com/");

    cout << response.body << endl;
}