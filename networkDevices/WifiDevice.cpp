#include <WiFi.h>
#include "WifiDevice.h"
#include "../PreferencesKeys.h"

RTC_NOINIT_ATTR char WiFiDevice_reconfdetect[17];

WifiDevice::WifiDevice(const String& hostname, Preferences* _preferences)
: NetworkDevice(hostname)
{
    _restartOnDisconnect = _preferences->getBool(preference_restart_on_disconnect);

    size_t caLength = _preferences->getString(preference_mqtt_ca,_ca,TLS_CA_MAX_SIZE);
    size_t crtLength = _preferences->getString(preference_mqtt_crt,_cert,TLS_CERT_MAX_SIZE);
    size_t keyLength = _preferences->getString(preference_mqtt_key,_key,TLS_KEY_MAX_SIZE);

    if(caLength > 1) // length is 1 when empty
    {
        Serial.println(F("MQTT over TLS."));
        Serial.println(_ca);
        _wifiClientSecure = new WiFiClientSecure();
        _wifiClientSecure->setCACert(_ca);
        if(crtLength > 1 && keyLength > 1) // length is 1 when empty
        {
            Serial.println(F("MQTT with client certificate."));
            Serial.println(_cert);
            Serial.println(_key);
            _wifiClientSecure->setCertificate(_cert);
            _wifiClientSecure->setPrivateKey(_key);
        }
        _mqttClient = new PubSubClient(*_wifiClientSecure);
    } else
    {
        Serial.println(F("MQTT without TLS."));
        _wifiClient = new WiFiClient();
        _mqttClient = new PubSubClient(*_wifiClient);
    }
}

PubSubClient *WifiDevice::mqttClient()
{
    return _mqttClient;
}

void WifiDevice::initialize()
{
    std::vector<const char *> wm_menu;
    wm_menu.push_back("wifi");
    wm_menu.push_back("exit");
    // reduced tieout if ESP is set to restart on disconnect
    _wm.setConfigPortalTimeout(_restartOnDisconnect ? 60 * 3 : 60 * 30);
    _wm.setShowInfoUpdate(false);
    _wm.setMenu(wm_menu);
    _wm.setHostname(_hostname);

    bool res = false;

    WiFiDevice_reconfdetect[16] = 0x00;

    if(strcmp(WiFiDevice_reconfdetect, "reconfigure_wifi") == 0)
    {
        Serial.println(F("Opening WiFi configuration portal."));
        memset(WiFiDevice_reconfdetect, 0, sizeof(WiFiDevice_reconfdetect));
        res = _wm.startConfigPortal();
    }
    else
    {
        res = _wm.autoConnect(); // password protected ap
    }

    if(!res) {
        Serial.println(F("Failed to connect. Wait for ESP restart."));
        delay(1000);
        ESP.restart();
    }
    else {
        Serial.print(F("WiFi connected: "));
        Serial.println(WiFi.localIP().toString());
    }

    if(_restartOnDisconnect)
    {
        _wm.setDisconnectedCallback([&]()
        {
            onDisconnected();
        });
    }

    _mqttClient->setBufferSize(_mqttMaxBufferSize);
}

void WifiDevice::reconfigure()
{
    strcpy(WiFiDevice_reconfdetect, "reconfigure_wifi");

    delay(200);
    ESP.restart();
}

void WifiDevice::printError()
{
    if(_wifiClientSecure != nullptr)
    {
        char lastError[100];
        _wifiClientSecure->lastError(lastError,100);
        Serial.println(lastError);
    }
    Serial.print(F("Free Heap: "));
    Serial.println(ESP.getFreeHeap());
}

bool WifiDevice::isConnected()
{
    return WiFi.isConnected();
}

bool WifiDevice::reconnect()
{
    delay(3000);
    return isConnected();
}

void WifiDevice::update()
{

}

void WifiDevice::onDisconnected()
{
    if(millis() > 60000)
    {
        ESP.restart();
    }
}
