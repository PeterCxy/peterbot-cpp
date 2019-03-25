#ifndef EVLOOP_HPP
#define EVLOOP_HPP
#include <sys/select.h>
#include <vector>

// A listener that registers interest on the event loop
// Every listener will be called on each iteration
// The listeners can provide their own set of file descriptors
// to be monitored on
struct evloop_listener {
    // Return whether the event loop should continue; that is,
    // would there be any potential future events. If this returns true,
    // the event loop will proceed even if no file descriptors can
    // be monitored right now.
    bool (*build_fdset)(fd_set *readfds, fd_set *writefds,
        fd_set *exceptfds, int *max_fd);
    bool (*on_loop)(fd_set *readfds, fd_set *writefds,
        fd_set *exceptfds);
};

class EvLoop {
    private:
        static EvLoop *sInstance;
        std::vector<evloop_listener> mListeners;
    public:
        static EvLoop *getDefault();
        static void init();
        EvLoop();
        void registerListener(struct evloop_listener listener);
        void run();
};

#endif