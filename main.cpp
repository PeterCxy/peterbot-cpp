#include "evloop.hpp"
#include "http.hpp"
#include <stdio.h>

int main() {
    EvLoop::init();
    HttpClient::init();
    HttpClient client;
    HttpClient client2;
    client.get("https://www.google.com")
        ->send([](HttpClient *client) {
            printf("=== Google ===\n");
            printf("%s\n", client->body());
        });
    client2.get("https://www.baidu.com")
        ->send([](HttpClient *client) {
            printf("=== Baidu ===\n");
            printf("%s\n", client->body());
        });
    EvLoop::getDefault()->run();
}