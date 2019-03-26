#include "bot.hpp"
#include <iostream>

PeterBot *PeterBot::sInstance = NULL;

void PeterBot::init(std::string name) {
    PeterBot::sInstance = new PeterBot(name);
}

PeterBot *PeterBot::getInstance() {
    return PeterBot::sInstance;
}

PeterBot::PeterBot(std::string name)
    : mClient { TelegramClient::getDefault()->clone() }
    , mSelfName { name } {
    
}

void PeterBot::run() {
    this->loop(0);
}

void PeterBot::loop(unsigned long offset) {
    this->mClient.getUpdates(offset, 60 * 5,
        [this, offset](TelegramClient *client, json *res, int code) {
            this->onUpdates(offset, res, code);
        });
}

void PeterBot::onUpdates(unsigned long offset, json *res, int code) {
    if (code != 200) {
        std::cerr << "Something happened during long-polling: " << code << std::endl;
        std::cerr << "Continuing anyway..." << std::endl;
        this->loop(offset);
        return;
    }
            
    unsigned long new_offset = offset;
    for (auto& upd : *res) {
        if (upd["update_id"].get<unsigned long>() > new_offset) {
            new_offset = upd["update_id"];
        }

        if (upd.contains("message")) {
            // This is a message
            json msg = upd["message"].get<json>();
            if (msg.contains("text")) {
                long chat_id = msg["chat"]["id"].get<long>();
                this->mClient.cloneOneTime() // Use a one-time client for this
                    ->sendMessageText(chat_id, msg["text"].get<std::string>(),
                        [this, new_offset](TelegramClient *client, json *res, int code) {
                            // Do nothing, this is an independent branch
                            // from the main event loop
                        }
                    );
            }
        }
    }

    this->loop(new_offset + 1);
}