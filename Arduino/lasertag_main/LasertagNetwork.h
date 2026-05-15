#ifndef LASERTAG_NETWORK_H
#define LASERTAG_NETWORK_H

#include <Arduino.h>
#include <esp_now.h>
#include <esp_wifi.h>

void addPeer(const uint8_t *mac);
void sendDiscovery();
void broadcastRankingTable();
void handleSendStatusLog();

#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
void onDataSent(const wifi_tx_info_t *info, esp_now_send_status_t status);
#else
void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);
#endif

#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 2
void onDataRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len);
#else
void onDataRecv(const uint8_t *mac, const uint8_t *data, int len);
#endif

#endif
