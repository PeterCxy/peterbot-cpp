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
        static bool onLoop(fd_set *readfds, fd_set *writefds,
            fd_set *exceptfds);
        void onFinish(CURLcode curlCode);
    public:
        static void init();
        HttpClient();
        ~HttpClient();
        std::string urlencode(const char* orig);
        HttpClient *get(const char *url);
        HttpClient *setTimeout(unsigned int sec);
        void send(HTTP_FINISH_CB cb);
        // Note: this is NOT a zero-terminated C-style string
        // you MUST use the length from `len`
        // For a proper body-to-string conversion, see `bodyStr()`
        char *body(size_t *len);
        // Get the string version of the body, without ever
        // worrying about zero-termination and length.
        std::string bodyStr();
        long status();
};

size_t http_client_curl_write(char *ptr, size_t size, size_t nmemb, void *userdata);

#endif