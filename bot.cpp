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
        [this, offset](TelegramClient *client, Telegram::Updates *res, int code) {
            this->onUpdates(offset, res, code);
        });
}

void PeterBot::onUpdates(unsigned long offset, Telegram::Updates *res, int code) {
    if (code != 200) {
        std::cerr << "Something happened during long-polling: " << code << std::endl;
        std::cerr << "Continuing anyway..." << std::endl;
        this->loop(offset);
        return;
    }
            
    unsigned long new_offset = offset;
    for (auto& upd : *res) {
        if (upd.update_id > new_offset) {
            new_offset = upd.update_id;
        }

        if (upd.message.has_value()) {
            // This is a message
            Telegram::Message msg = upd.message.value();
            if (msg.text.has_value()) {
                std::cout << "sending: " << msg.text.value() << std::endl;
                this->mClient.cloneOneTime() // Use a one-time client for this
                    ->sendMessageText(msg.chat.id, msg.text.value(),
                        [this, new_offset](TelegramClient *client, Telegram::Message *res, int code) {
                            // Do nothing, this is an independent branch
                            // from the main event loop
                            std::cout << "sent: " << res->text.value() << std::endl;
                        }
                    );
            }
        }
    }

    this->loop(new_offset + 1);
}