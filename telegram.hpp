#ifndef TELEGRAM_HPP
#define TELEGRAM_HPP
#include "http.hpp"
#include <map>
#include <functional>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class TelegramClient {
    typedef std::map<std::string, std::string> TelegramOptions;
    typedef std::function<void(TelegramClient*, json*, int)> TelegramCallback;
    private:
        static TelegramClient *sDefaultInstance;
        const char *mApiKey;
        HttpClient mHttpClient;
        TelegramClient(const char *apiKey);
        // Query string can be used both for GET
        // and for POST with www-form-urlencoded
        std::string buildQueryString(TelegramOptions options);
    public:
        static void init(const char *mApiKey);
        static TelegramClient *getDefault();
        TelegramClient clone();
        // Note: the JSON result passed to the callback is
        // temporary; if you would use it after the callback
        // returns, make a copy of the values of your concern
        void methodGet(const char *method, TelegramOptions options,
            TelegramCallback callback);
        // Some specialized methods
        void getMe(TelegramCallback callback);
        void getUpdates(unsigned long offset, unsigned int timeout,
            TelegramCallback callback);
};
#endif