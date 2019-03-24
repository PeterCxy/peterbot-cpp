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

void HttpClient::onLoop(fd_set *readfds, fd_set *writefds,
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

HttpClient *HttpClient::get(char *url) {
    curl_easy_setopt(this->mCurlHandle, CURLOPT_URL, url);
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
    // Perform anything we can do now.
    HttpClient::performOnce();
}

void HttpClient::onFinish(CURLcode curlCode) {
    this->mCurlCode = curlCode;
    curl_easy_getinfo(this->mCurlHandle, CURLINFO_RESPONSE_CODE, &this->mStatus);
    this->mFinishCb(this);
    curl_multi_remove_handle(HttpClient::sCurlHandleM, this->mCurlHandle);
    // Reset every option so that this client can be re-used in the future
    curl_easy_reset(this->mCurlHandle);
}

char *HttpClient::body() {
    return this->mBufferBody.data();
}

long HttpClient::status() {
    return this->mStatus;
}

size_t http_client_curl_write(char *ptr, size_t size, size_t nmemb, void *userdata) {
    std::vector<char> *buffer = (std::vector<char>*) userdata;
    std::copy(ptr, ptr + nmemb, std::back_inserter(*buffer));
    return nmemb;
}