#include "evloop.hpp"
#include "http.hpp"
#include "telegram.hpp"
#include "bot.hpp"
#include <stdlib.h>
#include <iostream>

int main() {
    EvLoop::init();
    HttpClient::init();

    // Read the API key
    const char* apiKey = getenv("TELEGRAM_API_KEY");
    if (apiKey == NULL) {
        std::cerr << "Please provide API key" << std::endl;
        return -1;
    }
    TelegramClient::init(apiKey);

    TelegramClient client = TelegramClient::getDefault()->clone();
    client.getMe([](TelegramClient *client, json *res, int code) {
        if (code != 200)
            return;
        auto name = (*res)["username"].get<std::string>();
        std::cout << "I am @" << name << std::endl;
        PeterBot::init(name);
        PeterBot::getInstance()->run();
    });

    EvLoop::getDefault()->run();
}