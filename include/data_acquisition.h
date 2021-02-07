//
// Created by development on 24.01.21.
//

#ifndef EDGE_ESP32_DATA_ACQUISITION_H
#define EDGE_ESP32_DATA_ACQUISITION_H
#include <Arduino.h>
#include <HTTPClient.h>
extern WiFiClient *wifiClientI2S ;
extern HTTPClient *httpClientI2S ;
void sendData(WiFiClient *wifiClient, HTTPClient *httpClient, const char *url, uint8_t *bytes, size_t count);
#endif //EDGE_ESP32_DATA_ACQUISITION_H
