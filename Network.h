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
    explicit Network(Preferences* preferences, const String& maintenancePathPrefix);

    void initialize();
    bool update();
    void registerMqttReceiver(MqttReceiver* receiver);
    void reconfigureDevice();
    void setMqttPresencePath(char* path);
    void disableAutoRestarts(); // disable on OTA start

    void subscribe(const char* path);
    void initTopic(const char* path, const char* value);
    void publishFloat(const char* topic, const float value, const uint8_t precision = 2);
    void publishInt(const char* topic, const int value);
    void publishUInt(const char* topic, const unsigned int value);
    void publishULong(const char* topic, const unsigned long value);
    void publishBool(const char* topic, const bool value);
    bool publishString(const char* topic, const char* value);
    void publishPin(const char* topic, int value);

    void publishPresenceDetection(char* csv);

    int mqttConnectionState(); // 0 = not connected; 1 = connected; 2 = connected and mqtt processed
    bool encryptionSupported();
    const String networkDeviceName() const;

    const NetworkDeviceType networkDeviceType();

    uint16_t subscribe(const char* topic, uint8_t qos);

    void setKeepAliveCallback(std::function<void()> reconnectTick);
    void addReconnectedCallback(std::function<void()> reconnectedCallback);

    bool comparePrefixedPath(const char* fullPath, const char* subPath);

private:
    static void onMqttDataReceivedCallback(const espMqttClientTypes::MessageProperties& properties, const char* topic, const uint8_t* payload, size_t len, size_t index, size_t total);
    void onMqttDataReceived(const espMqttClientTypes::MessageProperties& properties, const char* topic, const uint8_t* payload, size_t len, size_t index, size_t total);
    void setupDevice();
    bool reconnect();

    void onMqttConnect(const bool& sessionPresent);
    void onMqttDisconnect(const espMqttClientTypes::DisconnectReason& reason);

    void buildMqttPath(const char* path, char* outPath);

    static Network* _inst;
    Preferences* _preferences;
    String _hostname;
    char _hostnameArr[101] = {0};
    NetworkDevice* _device = nullptr;
    int _mqttConnectionState = 0;
    bool _connectReplyReceived = false;

    unsigned long _nextReconnect = 0;
    char _mqttBrokerAddr[101] = {0};
    char _mqttUser[31] = {0};
    char _mqttPass[31] = {0};
    char _mqttPresencePrefix[181] = {0};
    String _maintenancePathPrefix;
    char _mqttPath[181] = {0};
    int _networkTimeout = 0;
    std::vector<MqttReceiver*> _mqttReceivers;
    char* _presenceCsv = nullptr;
    bool _restartOnDisconnect = false;
    bool _firstConnect = true;
    bool _publishDebugInfo = false;
    std::vector<String> _subscribedTopics;
    std::map<String, String> _initTopics;

    unsigned long _lastConnectedTs = 0;
    unsigned long _lastMaintenanceTs = 0;
    unsigned long _lastRssiTs = 0;
    long _rssiPublishInterval = 0;
    std::function<void()> _keepAliveCallback = nullptr;
    std::vector<std::function<void()>> _reconnectedCallbacks;

    std::map<const char*, int> _pinStates;

    NetworkDeviceType _networkDeviceType  = (NetworkDeviceType)-1;

    int8_t _lastRssi = 127;
};
