#pragma once

#include <Preferences.h>
#include <vector>
#include <map>
#include "networkDevices/NetworkDevice.h"
#include "MqttReceiver.h"

enum class NetworkDeviceType
{
    WiFi,
    W5500
};

class Network
{
public:
    explicit Network(const NetworkDeviceType networkDevice, Preferences* preferences);

    void initialize();
    bool update();
    void registerMqttReceiver(MqttReceiver* receiver);
    void reconfigureDevice();

    void subscribe(const char* path);
    void publishFloat(const char* topic, const float value, const uint8_t precision = 2);
    void publishInt(const char* topic, const int value);
    void publishUInt(const char* topic, const unsigned int value);
    void publishBool(const char* topic, const bool value);
    bool publishString(const char* topic, const char* value);
    void publishPin(const char* topic, int value);
    bool comparePrefixedPath(const char* fullPath, const char* subPath);

    void publishPresenceDetection(char* csv);

    PubSubClient* mqttClient();
    bool isMqttConnected();

private:
    static void onMqttDataReceivedCallback(char* topic, byte* payload, unsigned int length);
    void onMqttDataReceived(char*& topic, byte*& payload, unsigned int& length);
    void setupDevice(const NetworkDeviceType hardware);
    bool reconnect();

    void buildMqttPath(const char* path, char* outPath);

    static Network* _inst;
    Preferences* _preferences;
    String _hostname;
    NetworkDevice* _device = nullptr;
    bool _mqttConnected = false;

    unsigned long _nextReconnect = 0;
    char _mqttBrokerAddr[101] = {0};
    char _mqttUser[31] = {0};
    char _mqttPass[31] = {0};
    char _mqttPath[181] = {0};
    char _mqttPresencePrefix[181] = {0};
    std::vector<String> _subscribedTopics;
    int _networkTimeout = 0;
    std::vector<MqttReceiver*> _mqttReceivers;
    char* _presenceCsv = nullptr;
    std::map<const char*, int> _pinStates;

    unsigned long _lastConnectedTs = 0;
    uint8_t _lastPublishTs = 0;
};
