#ifndef LASERTAG_CONFIG_H
#define LASERTAG_CONFIG_H

#include <Arduino.h>

constexpr uint8_t WIFI_CHANNEL = 1;
constexpr int MAX_PEERS = 20;
constexpr int MAX_PLAYERS = 10;

constexpr uint16_t IR_SEND_PIN = 4;
constexpr uint16_t IR_RECV_PIN = 14;
constexpr uint16_t BUTTON_PIN = 19;
constexpr uint16_t RGB_RED_PIN = 25;
constexpr uint16_t RGB_GREEN_PIN = 26;
constexpr uint16_t RGB_BLUE_PIN = 27;
constexpr bool RGB_COMMON_ANODE = false;

constexpr char MY_NAME[] = "PLAYER_1";
// Give each blaster a unique NEC command byte, e.g. PLAYER_1 = 0x01, PLAYER_2 = 0x02.
constexpr uint16_t IR_GAME_ADDRESS = 0x00FF;
constexpr uint8_t MY_IR_COMMAND = 0x01;

constexpr uint8_t MSG_DISCOVERY = 0;
constexpr uint8_t MSG_TABLE = 1;

constexpr unsigned long DEBOUNCE_MS = 50;
constexpr unsigned long SELF_HIT_IGNORE_MS = 200;
constexpr unsigned long HIT_DISABLE_MS = 5000;

#endif
