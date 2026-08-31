#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>

using std::size_t;

using BaseType_t = int;
using UBaseType_t = unsigned int;
using TickType_t = uint32_t;
using SemaphoreHandle_t = void *;
using TaskFunction_t = void (*)(void *);

constexpr BaseType_t pdPASS = 1;
constexpr TickType_t portMAX_DELAY = 0xFFFFFFFFU;

template <typename T>
constexpr T min(T left, T right) {
  return std::min(left, right);
}

struct HardwareSerial {
  void begin(unsigned long);
  void println(const char *);
  int printf(const char *, ...);
};

extern HardwareSerial Serial;

uint32_t millis();
void delay(unsigned long);
SemaphoreHandle_t xSemaphoreCreateMutex();
BaseType_t xSemaphoreTake(SemaphoreHandle_t, TickType_t);
BaseType_t xSemaphoreGive(SemaphoreHandle_t);
BaseType_t xTaskCreate(TaskFunction_t, const char *, uint32_t, void *,
                       UBaseType_t, void *);
void vTaskDelay(TickType_t);

#define pdMS_TO_TICKS(milliseconds) (static_cast<TickType_t>(milliseconds))
