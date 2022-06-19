#pragma once

#include <PubSubClient.h>
#include <WiFiClient.h>
#include <Preferences.h>
#include "SpiffsCookie.h"

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
    void publishFloat(const char* topic, const float value, const uint8_t precision = 2);
    void publishInt(const char* topic, const int value);
    void publishBool(const char* topic, const bool value);
    void publishString(const char* topic, const char* value);

    void buildMqttPath(const char* path, char* outPath);

    bool reconnect();

    PubSubClient _mqttClient;
    WiFiClient _wifiClient;
    Preferences* _preferences;
    SpiffsCookie _cookie;

    String _wifiSSID = "";
    String _wifiPSK = "";

    bool _mqttConnected = false;

    unsigned long _nextReconnect = 0;
    char _mqttBrokerAddr[101] = {0};
    char _mqttPath[181] = {0};
    char _mqttUser[31] = {0};
    char _mqttPass[31] = {0};

    char* _presenceCsv = nullptr;
};
