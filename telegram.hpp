#ifndef TELEGRAM_HPP
#define TELEGRAM_HPP
#include "http.hpp"
#include "tg_types.hpp"
#include <map>
#include <functional>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class TelegramClient {
    typedef std::map<std::string, std::string> TelegramOptions;
    typedef std::function<void(TelegramClient*, json*, int)> TelegramCallback;
    // Typed version of Telegram Callback functions
    template<typename T>
    using TelegramCallbackT =
        std::function<void(TelegramClient*, T*, int)>;
    private:
        static TelegramClient *sDefaultInstance;
        const char *mApiKey;
        bool mOneTime = false; // if true, suicide on finishing one request
        HttpClient mHttpClient;
        TelegramClient(const char *apiKey);
        // Query string can be used both for GET
        // and for POST with www-form-urlencoded
        std::string buildQueryString(TelegramOptions options);
        void onHttpResult(TelegramCallback callback);
    public:
        static void init(const char *mApiKey);
        static TelegramClient *getDefault();
        TelegramClient clone();
        // Produce a one-time heap-allocated client that commits suicide after
        // a single request is made and concluded
        TelegramClient *cloneOneTime();
        // Note: the JSON result passed to the callback is
        // temporary; if you would use it after the callback
        // returns, make a copy of the values of your concern
        void methodGet(const char *method, TelegramOptions options,
            TelegramCallback callback);
        void methodPost(const char *method, TelegramOptions options,
            TelegramCallback callback);
        // Typed versions of the above functions
        // templates should be in the header, so
        // we keep the original ones around in 
        // the actual cpp file
        template<typename T>
        void methodGetT(const char *method, TelegramOptions options,
            TelegramCallbackT<T> callback) {
            this->methodGet(method, options,
                [callback](TelegramClient *client, json *res, int code) {
                    if (res == NULL) {
                        callback(client, NULL, code);
                    } else {
                        T _res = T::from_json(res);
                        callback(client, &_res, code);
                    }
                });
        }
        template<typename T>
        void methodPostT(const char *method, TelegramOptions options,
            TelegramCallbackT<T> callback) {
            this->methodPost(method, options,
                [callback](TelegramClient *client, json *res, int code) {
                    if (res == NULL) {
                        callback(client, NULL, code);
                    } else {
                        T _res = T::from_json(res);
                        callback(client, &_res, code);
                    }
                });
        }
        // Some specialized methods
        void getMe(TelegramCallbackT<Telegram::User> callback);
        void getUpdates(unsigned long offset, unsigned int timeout,
            TelegramCallbackT<Telegram::Updates> callback);
        void sendMessageText(long chatId, std::string message,
            TelegramCallbackT<Telegram::Message> callback);
};
#endif