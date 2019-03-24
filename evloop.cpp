#include "evloop.hpp"
#include <cstddef>
#include <unistd.h>

EvLoop *EvLoop::sInstance = NULL;

void EvLoop::init() {
    EvLoop::sInstance = new EvLoop();
}

EvLoop *EvLoop::getDefault() {
    return EvLoop::sInstance;
}

EvLoop::EvLoop() {

}

void EvLoop::registerListener(struct evloop_listener listener) {
    this->mListeners.push_back(listener);
}

void EvLoop::run() {
    struct timeval timeout = {
        .tv_sec = 1,
    };
    fd_set readfds, writefds, exceptfds;
    bool should_continue = true;
    while (should_continue) {
        FD_ZERO(&readfds);
        FD_ZERO(&writefds);
        FD_ZERO(&exceptfds);
        int max_fd = -1;
        should_continue = false;
        for (auto& listener : this->mListeners) {
            int cur_max_fd = -1;
            should_continue |= 
                listener.build_fdset(&readfds, &writefds, &exceptfds, &cur_max_fd);
            if (cur_max_fd > max_fd)
                max_fd = cur_max_fd;
        }
        int select_res = -1;
        if (max_fd == -1) {
            // Sleep for a short amount of time
            usleep(1000);
            // There is no longer any fd remaining
            // However, we still loop and call our listeners
            // with empty fd_set, to ensure that
            // any remaining event will be called.
            // (this is as per requirement by libcurl)
            select_res = 0;
            // Clear all fds to signify that nothing can be done
            FD_ZERO(&readfds);
            FD_ZERO(&writefds);
            FD_ZERO(&exceptfds);
        } else {
            select_res = select(max_fd + 1, &readfds, &writefds, &exceptfds, &timeout);
        }

        if (select_res >= 0) {
            // Let all the registered event listeners do their job
            for (auto& listener : this->mListeners) {
                listener.on_loop(&readfds, &writefds, &exceptfds);
            }
        }
    }
}