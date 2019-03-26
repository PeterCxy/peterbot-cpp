#ifndef BOT_HPP
#define BOT_HPP
#include "telegram.hpp"

class PeterBot {
    private:
        static PeterBot *sInstance;
        TelegramClient mClient;
        std::string mSelfName;
        PeterBot(std::string name);
        // Inner function actually performing run()
        void loop(unsigned long offset);
        void onUpdates(unsigned long offset, json *res, int code);
    public:
        static void init(std::string name);
        static PeterBot *getInstance();
        // A tail-recursive asynchronous loop
        // to receive and process all messages
        // from Telegram
        void run();
};

#endif