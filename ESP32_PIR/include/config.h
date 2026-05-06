// config.h
// Этот файл содержит всё, что может меняться в зависимости от сети и брокера

#ifndef CONFIG_H
#define CONFIG_H

// Wi-Fi настройки 
const char* ssid = "SAV2";        // имя твоей Wi-Fi сети
const char* password = "z00442211Q";       // пароль

// MQTT брокер (на твоём компьютере или Raspberry Pi)
const char* mqtt_server = "192.168.0.103";  // IP твоего ПК (где запущен Mosquitto)
const int mqtt_port = 1883;                // стандартный порт MQTT

#endif