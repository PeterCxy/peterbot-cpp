#include "telegram.hpp"
#include "util.hpp"
#include <stdio.h>

#define TELEGRAM_API_FORMAT "https://api.telegram.org/bot%s/%s%s"

TelegramClient *TelegramClient::sDefaultInstance = NULL;

void TelegramClient::init(const char *apiKey) {
    TelegramClient::sDefaultInstance = new TelegramClient(apiKey);
}

TelegramClient *TelegramClient::getDefault() {
    return TelegramClient::sDefaultInstance;
}

TelegramClient::TelegramClient(const char *apiKey) {
    this->mApiKey = apiKey;
}

std::string TelegramClient::buildQueryString(TelegramOptions options) {
    std::string ret;
    ret.push_back('?');
    for (auto& entry : options) {
        ret.append(entry.first);
        ret.push_back('=');
        ret.append(this->mHttpClient.urlencode(entry.second.data()));
        ret.push_back('&');
    }
    // Remove the extra '&' at the end
    // if the options is empty, then this removes the "?"
    // so that we get empty string for empty options
    ret.pop_back();
    return ret;
}

TelegramClient TelegramClient::clone() {
    return TelegramClient(this->mApiKey);
}

void TelegramClient::methodGet(const char *method, TelegramOptions options,
        TelegramCallback callback) {
    std::string url = string_format(TELEGRAM_API_FORMAT, this->mApiKey,
        method, this->buildQueryString(options).data());
    this->mHttpClient.get(url.data())
        ->send([this, callback](HttpClient *client) {
            if (client->status() != 200) {
                callback(this, NULL, client->status());
            }
            json res = json::parse(client->body());
            if (!res["ok"]) {
                // Telegram error
                callback(this, &res, res["error_code"]);
            }
            json result_real = res["result"].get<json>();
            callback(this, &result_real, 200);
        });
}

void TelegramClient::getMe(TelegramCallback callback) {
    this->methodGet("getMe", {}, callback);
}

void TelegramClient::getUpdates(unsigned long offset, unsigned int timeout,
        TelegramCallback callback) {
    this->mHttpClient.setTimeout(timeout);
    this->methodGet("getUpdates", {
        {"offset", std::to_string(offset)},
        {"timeout", std::to_string(timeout)},
    }, callback);
}