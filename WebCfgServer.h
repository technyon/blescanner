#pragma once

#include <Preferences.h>
#include <WebServer.h>
#include "Ota.h"
#include "Network.h"

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
    WebCfgServer(Network* network, EthServer* ethServer, Preferences* preferences, bool allowRestartToPortal);
    ~WebCfgServer() = default;

    void initialize();
    void update();


private:
    bool processArgs(String& message);
    void buildHtml(String& response);
    void buildCredHtml(String& response);
    void buildOtaHtml(String& response);
    void buildMqttConfigHtml(String& response);
    void buildConfirmHtml(String& response, const String &message, uint32_t redirectDelay = 5);
    void buildConfigureWifiHtml(String& response);
    void sendNewCss();
    void sendFontsInterMinCss();
    void sendFavicon();

    void buildHtmlHeader(String& response);
    void printInputField(String& response, const char* token, const char* description, const char* value, const size_t maxLength, const bool isPassword = false);
    void printInputField(String& response, const char* token, const char* description, const int value, size_t maxLength);
    void printCheckBox(String& response, const char* token, const char* description, const bool value);
    void printTextarea(String& response, const char *token, const char *description, const char *value, const size_t maxLength);
    void buildNavigationButton(String& response, const char* caption, const char* targetPath);

    void printParameter(String& response, const char* description, const char* value);

    void waitAndProcess(const bool blocking, const uint32_t duration);
    void handleOtaUpload();

    WebServer _server;
    Network* _network;
    Preferences* _preferences;
    Ota _ota;
    String _hostname;

    bool _hasCredentials = false;
    char _credUser[31] = {0};
    char _credPassword[31] = {0};
    bool _allowRestartToPortal = false;
    uint32_t _transferredSize = 0;
    bool _otaStart = true;

    String _confirmCode = "----";

    bool _enabled = true;
};