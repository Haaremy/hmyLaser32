#include "ZoneIr.h"
#include "Config.h"

namespace {

constexpr uint8_t ZONE_COUNT = 3;
constexpr uint8_t MAX_DURATIONS = 90;
constexpr uint32_t FRAME_GAP_US = 12000;

struct ZoneCapture {
  uint8_t pin;
  uint8_t zone;
  bool enabled;
  volatile bool active;
  volatile bool ready;
  volatile bool overflow;
  volatile uint32_t frameStartUs;
  volatile uint32_t lastEdgeUs;
  volatile uint8_t count;
  volatile uint16_t durations[MAX_DURATIONS];
  volatile uint8_t levels[MAX_DURATIONS];
};

ZoneCapture captures[ZONE_COUNT] = {
  { IR_RECV_PIN, IR_RECV_ZONE, true, false, false, false, 0, 0, 0, {}, {} },
  { IR_RECV_PIN_SECONDARY, IR_RECV_ZONE_SECONDARY, HAS_ZONE2_RECEIVER, false, false, false, 0, 0, 0, {}, {} },
  { IR_RECV_PIN_TERTIARY, IR_RECV_ZONE_TERTIARY, HAS_ZONE3_RECEIVER, false, false, false, 0, 0, 0, {}, {} }
};

portMUX_TYPE captureMux = portMUX_INITIALIZER_UNLOCKED;

bool approx(uint32_t value, uint32_t target, uint32_t tolerance) {
  return value >= target - tolerance && value <= target + tolerance;
}

void IRAM_ATTR onZoneEdge(void *arg) {
  ZoneCapture *cap = static_cast<ZoneCapture *>(arg);
  const uint32_t now = micros();
  const uint8_t levelNow = digitalRead(cap->pin);

  portENTER_CRITICAL_ISR(&captureMux);

  if (cap->lastEdgeUs == 0) {
    cap->active = (levelNow == LOW);
    cap->ready = false;
    cap->overflow = false;
    cap->count = 0;
    cap->frameStartUs = now;
    cap->lastEdgeUs = now;
    portEXIT_CRITICAL_ISR(&captureMux);
    return;
  }

  const uint32_t duration = now - cap->lastEdgeUs;
  const uint8_t previousLevel = levelNow == LOW ? HIGH : LOW;

  if (duration > FRAME_GAP_US && levelNow == LOW) {
    cap->active = true;
    cap->ready = false;
    cap->overflow = false;
    cap->count = 0;
    cap->frameStartUs = now;
    cap->lastEdgeUs = now;
    portEXIT_CRITICAL_ISR(&captureMux);
    return;
  }

  if (cap->active && !cap->ready) {
    if (cap->count < MAX_DURATIONS) {
      cap->durations[cap->count] = duration > 0xFFFFu ? 0xFFFFu : (uint16_t)duration;
      cap->levels[cap->count] = previousLevel;
      cap->count++;
    } else {
      cap->overflow = true;
      cap->ready = true;
      cap->active = false;
    }
  }

  cap->lastEdgeUs = now;
  portEXIT_CRITICAL_ISR(&captureMux);
}

bool decodeNec(const uint16_t *durations, const uint8_t *levels, uint8_t count, uint32_t &raw) {
  for (uint8_t start = 0; start + 65 < count; start++) {
    if (levels[start] != LOW || levels[start + 1] != HIGH) continue;
    if (!approx(durations[start], 9000, 1800) || !approx(durations[start + 1], 4500, 1200)) continue;

    uint32_t value = 0;
    bool valid = true;
    for (uint8_t bit = 0; bit < 32; bit++) {
      const uint8_t markIdx = start + 2 + bit * 2;
      const uint8_t spaceIdx = markIdx + 1;
      if (levels[markIdx] != LOW || levels[spaceIdx] != HIGH) {
        valid = false;
        break;
      }
      if (!approx(durations[markIdx], 560, 300)) {
        valid = false;
        break;
      }
      const uint16_t space = durations[spaceIdx];
      if (!approx(space, 560, 350) && !approx(space, 1690, 650)) {
        valid = false;
        break;
      }
      value = (value << 1) | (space > 1000 ? 1u : 0u);
    }
    if (valid) {
      raw = value;
      return true;
    }
  }
  return false;
}

bool popFrame(uint8_t index, ZoneIrFrame &frame) {
  ZoneCapture &cap = captures[index];
  if (!cap.enabled) return false;

  uint16_t durations[MAX_DURATIONS];
  uint8_t levels[MAX_DURATIONS];
  uint8_t count = 0;
  uint32_t startUs = 0;
  bool ready = false;
  bool overflow = false;

  portENTER_CRITICAL(&captureMux);
  if (cap.active && !cap.ready && cap.lastEdgeUs != 0 && (uint32_t)(micros() - cap.lastEdgeUs) > FRAME_GAP_US) {
    cap.ready = true;
    cap.active = false;
  }
  ready = cap.ready;
  if (ready) {
    count = cap.count;
    startUs = cap.frameStartUs;
    overflow = cap.overflow;
    for (uint8_t i = 0; i < count; i++) {
      durations[i] = cap.durations[i];
      levels[i] = cap.levels[i];
    }
    cap.ready = false;
    cap.active = false;
    cap.overflow = false;
    cap.count = 0;
    cap.frameStartUs = 0;
    cap.lastEdgeUs = 0;
  }
  portEXIT_CRITICAL(&captureMux);

  if (!ready) return false;
  if (overflow) {
    Serial.printf("[IR:GPIO%u->ZONE%u] overflow while decoding\n",
                  (unsigned)cap.pin,
                  (unsigned)cap.zone);
    return false;
  }

  uint32_t raw = 0;
  if (!decodeNec(durations, levels, count, raw)) {
    Serial.printf("[IR:GPIO%u->ZONE%u] decode failed edges=%u first=%u/%u second=%u/%u\n",
                  (unsigned)cap.pin,
                  (unsigned)cap.zone,
                  (unsigned)count,
                  count > 0 ? (unsigned)durations[0] : 0,
                  count > 0 ? (unsigned)levels[0] : 0,
                  count > 1 ? (unsigned)durations[1] : 0,
                  count > 1 ? (unsigned)levels[1] : 0);
    return false;
  }
  frame.zone = cap.zone;
  frame.pin = cap.pin;
  frame.raw = raw;
  frame.startUs = startUs;
  return true;
}

}  // namespace

void zoneIrBegin() {
  for (uint8_t i = 0; i < ZONE_COUNT; i++) {
    if (!captures[i].enabled) continue;
    pinMode(captures[i].pin, INPUT_PULLUP);
    attachInterruptArg(digitalPinToInterrupt(captures[i].pin), onZoneEdge, &captures[i], CHANGE);
    Serial.printf("[IR] GPIO%u mapped to ZONE%u\n",
                  (unsigned)captures[i].pin,
                  (unsigned)captures[i].zone);
  }
}

bool zoneIrPoll(ZoneIrFrame &frame) {
  for (uint8_t i = 0; i < ZONE_COUNT; i++) {
    if (popFrame(i, frame)) return true;
  }
  return false;
}
