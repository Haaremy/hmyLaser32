#ifndef NFC_WRITER_CONFIG_H
#define NFC_WRITER_CONFIG_H

#include <Arduino.h>

constexpr char AP_SSID_PREFIX[] = "hmyLaser32-NFC-";
constexpr char AP_PASSWORD[] = "";
const IPAddress AP_IP(192, 168, 4, 1);
const IPAddress AP_GATEWAY(192, 168, 4, 1);
const IPAddress AP_NETMASK(255, 255, 255, 0);

constexpr uint16_t NFC_SS_PIN = 32;    // RC522 SDA / SS
constexpr uint16_t NFC_SCK_PIN = 33;
constexpr uint16_t NFC_MOSI_PIN = 25;
constexpr uint16_t NFC_MISO_PIN = 26;
constexpr uint16_t NFC_RST_PIN = 27;

// Mifare Classic data blocks. Sector trailers (3, 7, 11, ...) are skipped.
constexpr uint8_t NFC_DATA_BLOCKS[] = {4, 5, 6, 8, 9};
constexpr size_t NFC_DATA_BLOCK_COUNT = sizeof(NFC_DATA_BLOCKS) / sizeof(NFC_DATA_BLOCKS[0]);
constexpr size_t NFC_MAX_CONTENT_BYTES = NFC_DATA_BLOCK_COUNT * 16;

#endif
