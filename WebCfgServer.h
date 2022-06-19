#pragma once

#include <Preferences.h>
#include <WebServer.h>
#include "Network.h"
#include "Ota.h"

enum class TokenType
{
    None,
    MqttServer,
    MqttPort,
    MqttUser,
    MqttPass,
    MqttPath,
    QueryIntervalLockstate,
    QueryIntervalBattery,
};

class WebCfgServer
{
public:
    WebCfgServer(Network* network, Preferences* preferences);
    ~WebCfgServer() = default;

    void initialize();
    void update();


private:
    bool processArgs(String& message);
    void buildHtml(String& response);
    void buildCredHtml(String& response);
    void buildOtaHtml(String& response);
    void buildConfirmHtml(String& response, const String &message, uint32_t redirectDelay = 5);
    void buildConfigureWifiHtml(String& response);

    void buildHtmlHeader(String& response);
    void printInputField(String& response, const char* token, const char* description, const char* value, const size_t maxLength, const bool isPassword = false);
    void printInputField(String& response, const char* token, const char* description, const int value, size_t maxLength);

    void printParameter(String& response, const char* description, const char* value);

    void waitAndProcess(const bool blocking, const uint32_t duration);
    void handleOtaUpload();

    WebServer _server;
    Network* _network;
    Preferences* _preferences;
    Ota _ota;

    bool _hasCredentials = false;
    char _credUser[20] = {0};
    char _credPassword[20] = {0};
    uint32_t _transferredSize = 0;

    bool _enabled = true;
};