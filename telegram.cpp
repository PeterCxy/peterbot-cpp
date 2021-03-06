#include "telegram.hpp"
#include "util.hpp"
#include <stdio.h>

#define TELEGRAM_API_FORMAT "https://api.telegram.org/bot%s/%s%s"
#define DEFAULT_TIMEOUT 60

TelegramClient *TelegramClient::sDefaultInstance = NULL;

void TelegramClient::init(const char *apiKey) {
    TelegramClient::sDefaultInstance = new TelegramClient(apiKey);
}

TelegramClient *TelegramClient::getDefault() {
    return TelegramClient::sDefaultInstance;
}

TelegramClient::TelegramClient(const char *apiKey) {
    this->mApiKey = apiKey;
    this->mHttpClient.setTimeout(DEFAULT_TIMEOUT);
}

std::string TelegramClient::buildQueryString(TelegramOptions options) {
    std::string ret;
    ret.push_back('?');
    for (auto& entry : options) {
        ret.append(entry.first);
        ret.push_back('=');
        ret.append(this->mHttpClient.urlencode(entry.second.c_str()));
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

TelegramClient *TelegramClient::cloneOneTime() {
    TelegramClient *ret = new TelegramClient(this->mApiKey);
    ret->mOneTime = true;
    return ret;
}

void TelegramClient::methodGet(const char *method, TelegramOptions options,
        TelegramCallback callback) {
    std::string qstr = this->buildQueryString(options);
    std::string url = string_format(TELEGRAM_API_FORMAT, this->mApiKey,
        method, qstr.c_str());
    this->mHttpClient.get(url.c_str())
        ->send([this, callback](HttpClient *client) {
            this->onHttpResult(callback);
        });
}

void TelegramClient::methodPost(const char *method, TelegramOptions options,
        TelegramCallback callback) {
    // `x-www-form-urlencoded` is just query string without the ?
    // added in the post body
    std::string qstr = this->buildQueryString(options);
    qstr = qstr.substr(1, qstr.size() - 1); // Remove the ? of the query string
    std::string url = string_format(TELEGRAM_API_FORMAT, this->mApiKey, method, "");
    this->mHttpClient.post(url.c_str())
        ->formData(qstr.c_str(), qstr.size())
        ->send([this, callback](HttpClient *client) {
            this->onHttpResult(callback);
        });
}

void TelegramClient::onHttpResult(TelegramCallback callback) {
    if (this->mHttpClient.status() != 200) {
        callback(this, NULL, this->mHttpClient.status());
    }
    json res = json::parse(this->mHttpClient.bodyStr());
    if (!res["ok"]) {
        // Telegram error
        callback(this, &res, res["error_code"]);
    }
    json result_real = res["result"].get<json>();
    callback(this, &result_real, 200);
    // Suicide if this is a one-time 
    if (this->mOneTime) {
        delete this;
        return;
    }
    // Reset the timeout to default
    // since HttpClient should have already reset
    // by the time this callback is called, this
    // option will be preserved
    this->mHttpClient.setTimeout(DEFAULT_TIMEOUT);
}

void TelegramClient::getMe(TelegramCallbackT<Telegram::User> callback) {
    this->methodGetT("getMe", {}, callback);
}

void TelegramClient::getUpdates(unsigned long offset, unsigned int timeout,
        TelegramCallbackT<Telegram::Updates> callback) {
    // Connection timeout should be longer than long-polling timeout
    this->mHttpClient.setTimeout(timeout + 1);
    this->methodGetT("getUpdates", {
        {"offset", std::to_string(offset)},
        {"timeout", std::to_string(timeout)},
    }, callback);
}

void TelegramClient::sendMessageText(long chatId, std::string message,
        TelegramCallbackT<Telegram::Message> callback) {
    this->methodPostT("sendMessage", {
        {"chat_id", std::to_string(chatId)},
        {"text", message},
    }, callback);
}