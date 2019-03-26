#include "http.hpp"
#include "evloop.hpp"
#include <cstring>
#include <stdio.h>

CURLM *HttpClient::sCurlHandleM = NULL;
std::map<size_t, HttpClient*> HttpClient::sClients;

void HttpClient::init() {
    curl_global_init(CURL_GLOBAL_DEFAULT);
    HttpClient::sCurlHandleM = curl_multi_init();
    EvLoop::getDefault()->registerListener((struct evloop_listener) {
        .build_fdset = &HttpClient::buildFdset,
        .on_loop = &HttpClient::onLoop,
    });
}

int HttpClient::performOnce() {
    int still_running = 0;
    curl_multi_perform(HttpClient::sCurlHandleM, &still_running);
    return still_running;
}

bool HttpClient::buildFdset(fd_set *readfds, fd_set *writefds,
        fd_set *exceptfds, int *max_fd) {
    // As how libcurl behaves, we have to continue performing the request
    // as long as still_running is not 0, even if the last curl_multi_fdset
    // returned -1 for the max_fd.
    int still_running = HttpClient::performOnce();
    curl_multi_fdset(HttpClient::sCurlHandleM, readfds, writefds, exceptfds, max_fd);
    return still_running > 0;
}

bool HttpClient::onLoop(fd_set *readfds, fd_set *writefds,
        fd_set *exceptfds) {
    // Read all messages available from CURL
    struct CURLMsg *msg;
    int msgq;
    do {
        msg = curl_multi_info_read(HttpClient::sCurlHandleM, &msgq);
        if (msg && (msg->msg == CURLMSG_DONE)) {
            // Only DONE message is processed for now.
            size_t client_handle = (size_t) msg->easy_handle;
            if (HttpClient::sClients.find(client_handle) != HttpClient::sClients.end())
                HttpClient::sClients[client_handle]->onFinish(msg->data.result);
        }
    } while (msg);
    // onFinish() could have triggered more events
    // if so, we have to notify the event loop to continue.
    return HttpClient::performOnce() > 0;
}

HttpClient::HttpClient() {
    this->mCurlHandle = curl_easy_init();
    HttpClient::sClients[(size_t) this->mCurlHandle] = this;
}

HttpClient::~HttpClient() {
    curl_multi_remove_handle(HttpClient::sCurlHandleM, this->mCurlHandle);
    curl_easy_cleanup(this->mCurlHandle);
    HttpClient::sClients.erase((size_t) this->mCurlHandle);
}

HttpClient *HttpClient::get(const char *url) {
    curl_easy_setopt(this->mCurlHandle, CURLOPT_URL, url);
    curl_easy_setopt(this->mCurlHandle, CURLOPT_HTTPGET, 1);
    return this;
}

HttpClient *HttpClient::post(const char *url) {
    curl_easy_setopt(this->mCurlHandle, CURLOPT_URL, url);
    curl_easy_setopt(this->mCurlHandle, CURLOPT_POST, 1);
    return this;
}

HttpClient *HttpClient::formData(const char *data, size_t len) {
    // Set size BEFORE the data!
    curl_easy_setopt(this->mCurlHandle, CURLOPT_POSTFIELDSIZE, len);
    curl_easy_setopt(this->mCurlHandle, CURLOPT_COPYPOSTFIELDS, data);
    return this;
}

HttpClient *HttpClient::setTimeout(unsigned int sec) {
    curl_easy_setopt(this->mCurlHandle, CURLOPT_TIMEOUT, sec);
    return this;
}

void HttpClient::send(HTTP_FINISH_CB cb) {
    // Make sure we clear all the buffered data here
    this->mBufferBody.clear();
    this->mCurlCode = CURLE_OK;
    this->mStatus = -1;
    // Set the callback
    this->mFinishCb = cb;
    // Make sure we capture all the data :)
    curl_easy_setopt(this->mCurlHandle, CURLOPT_WRITEFUNCTION, http_client_curl_write);
    curl_easy_setopt(this->mCurlHandle, CURLOPT_WRITEDATA, &this->mBufferBody);
    // Begin polling on the client
    curl_multi_add_handle(HttpClient::sCurlHandleM, this->mCurlHandle);
}

void HttpClient::onFinish(CURLcode curlCode) {
    this->mCurlCode = curlCode;
    curl_easy_getinfo(this->mCurlHandle, CURLINFO_RESPONSE_CODE, &this->mStatus);
    // Do clean-up before we call the callback, in case the callback
    // might begin to re-use this HttpClient
    curl_multi_remove_handle(HttpClient::sCurlHandleM, this->mCurlHandle);
    // Reset every option so that this client can be re-used in the future
    curl_easy_reset(this->mCurlHandle);
    this->mFinishCb(this);
}

char *HttpClient::body(size_t *len) {
    *len = this->mBufferBody.size();
    return this->mBufferBody.data();
}

std::string HttpClient::bodyStr() {
    size_t len = 0;
    char *data = this->body(&len);
    return std::string(data, len);
}

long HttpClient::status() {
    return this->mStatus;
}

std::string HttpClient::urlencode(const char* orig) {
    char *encoded = curl_easy_escape(this->mCurlHandle, orig, 0);
    std::string ret = std::string(encoded);
    curl_free(encoded);
    return ret;
}

size_t http_client_curl_write(char *ptr, size_t size, size_t nmemb, void *userdata) {
    std::vector<char> *buffer = (std::vector<char>*) userdata;
    std::copy(ptr, ptr + nmemb, std::back_inserter(*buffer));
    return nmemb;
}