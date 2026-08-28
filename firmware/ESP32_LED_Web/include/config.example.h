// config.example.h
// ============================================================
//  ПРИМЕР конфигурации проекта ESP32_LED_Web (БЕЗ секретов,
//  кроме твоих WiFi-данных — их всё равно не следует пушить).
//
//  Как использовать:
//   1. Скопируй в config.h:  cp include/config.example.h include/config.h
//   2. Впиши свои WIFI_SSID / WIFI_PASSWORD.
//
//  ВАЖНО: config.h в git НЕ попадает (.gitignore).
// ============================================================

#ifndef CONFIG_H
#define CONFIG_H

// ---- WiFi (твоя домашняя сеть) ----
#define WIFI_SSID "TBOY"
#define WIFI_PASSWORD "12345678"

// ---- Лента WS2812B (адресуемая) ----
#define LED_PIN 27
#define NUM_LEDS 60
#define LED_BRIGHTNESS 100

// ---- Эквалайзер: сколько "столбиков" (полос частот) ----
#define BANDS 16

// ---- Звуковой датчик (цифровой, 3 пина: SIG / VCC / GND) ----
#define SOUND_PIN 14
// ВАЖНО: питай датчик от 3.3В, тогда SIG безопасен для ESP32.
// Если от 5В — делитель 1k+2k на SIG.
#define WINDOW_MS 250   // окно подсчёта срабатываний (мс)
#define MAX_BEATS 8     // столько за окно = максимальная яркость
#define DEBOUNCE_MS 30  // антидребезг (мс)

#endif
