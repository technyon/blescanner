#pragma once

#include <PubSubClient.h>
#include <WiFiClient.h>
#include <Preferences.h>
#include "SpiffsCookie.h"
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager

class Network
{
public:
    explicit Network(Preferences* preferences);
    virtual ~Network() = default;

    void initialize();
    void update();

    bool isMqttConnected();

    void publishPresenceDetection(char* csv);

    void restartAndConfigureWifi();

private:
    static Network* _inst;

    static void onMqttDataReceivedCallback(char* topic, byte* payload, unsigned int length);
    void onMqttDataReceived(char*& topic, byte*& payload, unsigned int& length);
    bool comparePrefixedPath(const char* fullPath, const char* subPath);

    void publishFloat(const char* topic, const float value, const uint8_t precision = 2);
    void publishInt(const char* topic, const int value);
    void publishBool(const char* topic, const bool value);
    void publishString(const char* topic, const char* value);
    void subscribe(const char* path);

    void buildMqttPath(const char* path, char* outPath);

    void onDisconnected();

    bool reconnect();

    PubSubClient _mqttClient;
    WiFiClient _wifiClient;
    Preferences* _preferences;
    SpiffsCookie _cookie;

    bool _mqttConnected = false;

    unsigned long _nextReconnect = 0;
    char _mqttBrokerAddr[101] = {0};
    char _mqttPath[181] = {0};
    char _mqttUser[31] = {0};
    char _mqttPass[31] = {0};
    bool _restartOnDisconnect = false;
    WiFiManager _wm;

    char* _presenceCsv = nullptr;
};
