#include <iostream>
#include <fstream>
#include <string>
#include <string_view>
#include <regex>
#include <thread>
#include <chrono>
#include "co_helper.hpp"
#include "http_client.hpp"

using namespace std;

// curl https://api.openai.com/v1/completions \
// -H "Content-Type: application/json" \
// -H "Authorization: Bearer YOUR_API_KEY" \
// -d '{"model": "text-davinci-003", "prompt": "Say this is a test", "temperature": 0, "max_tokens": 2000}'

void initialize()
{
    ofstream log("client.log", ios::app);

    clog.rdbuf(log.rdbuf());
}

int main(int argc, char *argv[])
{
    //initialize();

    //std::this_thread::sleep_for(std::chrono::seconds(1));

    if (argc != 2)
    {
        printf("Usage: client url\n");
        return -1;
    }

    try
    {
        auto http_client = http::Client::create();
        auto url = argv[1]; //"https://cn.bing.com:443/search?q=hello"

        cout << "A http request is getting started." << endl;
        auto response = http_client->await_get(url).get();

        cout << response.body << endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
    }
    catch (...)
    {
        std::cerr << "caught an unkonwn execption." << '\n';
    }

    return 0;
}