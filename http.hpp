#ifndef HTTP_HPP
#define HTTP_HPP
#include <curl/curl.h>
#include <sys/select.h>
#include <map>
#include <vector>
#include <functional>

class HttpClient {
    typedef std::function<void(HttpClient*)> HTTP_FINISH_CB;
    private:
        static CURLM *sCurlHandleM;
        static std::map<size_t, HttpClient*> sClients;
        CURL *mCurlHandle;
        // Buffer for the body of a response
        std::vector<char> mBufferBody;
        // Result code of the curl client
        CURLcode mCurlCode;
        // HTTP response code
        long mStatus;
        // The callback yet to be called upon the request get finished
        HTTP_FINISH_CB mFinishCb;
        static int performOnce();
        // Callbacks used by EvLoop
        static bool buildFdset(fd_set *readfds, fd_set *writefds,
            fd_set *exceptfds, int *max_fd);
        static void onLoop(fd_set *readfds, fd_set *writefds,
            fd_set *exceptfds);
        void onFinish(CURLcode curlCode);
    public:
        static void init();
        HttpClient();
        ~HttpClient();
        HttpClient *get(char *url);
        void send(HTTP_FINISH_CB cb);
        char *body();
        long status();
};

size_t http_client_curl_write(char *ptr, size_t size, size_t nmemb, void *userdata);

#endif