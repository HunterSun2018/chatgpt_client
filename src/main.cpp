#include <iostream>
#include <fstream>
#include <string>
#include <string_view>
#include <regex>
#include <thread>
#include <chrono>
#include <readline/readline.h>
#include <readline/history.h>
#include <fmt/format.h>
#include <boost/json/src.hpp>
#include "co_helper.hpp"
#include "http_client.hpp"

using namespace std;

Task<void> run_chat_gpt(Http::client_ptr client, string_view prompt);
string ChatGPT_KEY;

namespace color
{
    const std::string RED("\033[0;31m");
    const std::string GREEN("\033[1;32m");
    const std::string YELLOW("\033[1;33m");
    const std::string CYAN("\033[0;36m");
    const std::string MAGENTA("\033[0;35m");
    const std::string RESET("\033[0m");
} // namespace color

void initialize()
{
    ofstream log("client.log", ios::app);

    clog.rdbuf(log.rdbuf());
}

int main(int argc, char *argv[])
{
    try
    {
        // initialize();

        std::this_thread::sleep_for(std::chrono::seconds(0));
        if (auto key = getenv("ChatGPT_KEY"); key)
            ChatGPT_KEY = key;
        else
            throw runtime_error("Please run \"export ChatGPT_KEY='Your Key'\" first.");

        auto http_client = Http::Client::create();
        string prompt;

        do
        {
            cout << color::YELLOW << "Please input prompts for ChatGPT :\n" << color::RESET << endl;
            char *cmd = readline("");

            prompt = cmd;
            if (prompt == "quit")
                break;

            add_history(cmd);

            run_chat_gpt(http_client, prompt).get();

            free(cmd);
        } while (true);
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

void run_test(string_view url)
{
    auto http_client = Http::Client::create();
    // auto url = "https://cn.bing.com:443/search?q=hello"

    cout << "A http request is getting started." << endl;
    auto response = http_client->await_get(url).get();

    cout << response.body << endl;
}
std::size_t replace_all(std::string &inout, std::string_view what, std::string_view with)
{
    std::size_t count{};
    for (std::string::size_type pos{};
         inout.npos != (pos = inout.find(what.data(), pos, what.length()));
         pos += with.length(), ++count)
    {
        inout.replace(pos, what.length(), with.data(), with.length());
    }

    // inout.erase(0, inout.find_first_not_of('\n'));

    return count;
}
// curl https://api.openai.com/v1/completions \
// -H "Content-Type: application/json" \
// -H "Authorization: Bearer YOUR_API_KEY" \
// -d '{"model": "text-davinci-003", "prompt": "Say this is a test", "temperature": 0, "max_tokens": 2000}'
Task<void> run_chat_gpt(Http::client_ptr client, string_view prompt)
{
    string url = "https://api.openai.com/v1/completions";

    using namespace boost::json;
    object obj;
    obj["model"] = "text-davinci-003";
    obj["prompt"] = prompt;
    obj["temperature"] = 0;
    obj["max_tokens"] = 2000;
    ostringstream oss;
    oss << obj;

    Http::Request req;
    req.header["Content-Type"] = "application/json";
    req.header["Authorization"] = "Bearer " + ChatGPT_KEY;
    req.body = oss.str();

    try
    {
        auto response = co_await client->await_post(url, req, 120);

        // cout << response.body << endl;
        if (response.body.empty())
            co_return;

        value result = parse(response.body);
        auto obj = result.as_object();
        if (obj["error"].is_null())
        {
            for (auto item : obj["choices"].as_array())
            {
                std::string text = item.as_object()["text"].as_string().data();

                replace_all(text, "\\n", "\n");

                cout << "\n"
                     << color::GREEN << "ChatGPT : " << color::RESET << text
                     << "\n"
                     << endl;
            }
        }
        else
        {
            cout << "\nChatGPT : " << obj["error"].as_object()["message"].as_string() << endl;
        }
    }
    catch (const boost::system::system_error &ec)
    {
        std::string error  = ec.what();
        auto pos = error.find_first_of('[');
        
        if(pos != std::string::npos)
            error = error.substr(0, pos);

        std::cerr << ec.what() << endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
    }

    co_return;
}