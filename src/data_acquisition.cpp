//
// Created by development on 24.01.21.
//

#include "data_acquisition.h"

// send data to a remote address

void sendData(WiFiClient *wifiClient, HTTPClient *httpClient, const char *url, uint8_t *bytes, size_t count) {
    // send them off to the server
    digitalWrite(LED_BUILTIN, HIGH);
    httpClient->begin(*wifiClientI2S, url);
    httpClient->addHeader("content-type", "application/octet-stream");
    httpClient->POST(bytes, count);
    httpClient->end();
    digitalWrite(LED_BUILTIN, LOW);
}
