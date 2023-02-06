#include <iostream>
#include <string_view>
#include <gtest/gtest.h>
#include "http_client.hpp"

using namespace std;

TEST(MyTestSuitName, MyTestCaseName)
{

    int actual = 1;
    EXPECT_GT(actual, 0);
    EXPECT_EQ(1, actual) << "Should be equal to one";
}

TEST(MyTestSuitName, HttpClientCase)
{
    auto code = [](string_view url) -> Task<int>
    {
        auto http_client = http::Client::create();

        auto response = co_await http_client->await_get(url);

        co_return response.status_code;
    }("http://bing.com")
                                           .get();

    EXPECT_EQ(200, code) << "Should be equal to http statue code 200";
}

int main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}