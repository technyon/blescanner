#include "Network.h"
#include "WiFi.h"
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
#include "Arduino.h"
#include "MqttTopics.h"
#include "PreferencesKeys.h"

Network::Network(Preferences* preferences)
: _mqttClient(_wifiClient),
  _preferences(preferences)
{}

void Network::initialize()
{
    WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP
    // it is a good practice to make sure your code sets wifi mode how you want it.

    //WiFiManager, Local intialization. Once its business is done, there is no need to keep it around
    WiFiManager wm;

    // reset settings - wipe stored credentials for testing
    // these are stored by the esp library
    //wm.resetSettings();

    bool res = wm.autoConnect(); // password protected ap

    if(!res) {
        Serial.println(F("Failed to connect"));
        return;
        // ESP.restart();
    }
    else {
        //if you get here you have connected to the WiFi
        Serial.println(F("connected...yeey :)"));
    }

    const char* brokerAddr = _preferences->getString(preference_mqtt_broker).c_str();
    strcpy(_mqttBrokerAddr, brokerAddr);

    int port = _preferences->getInt(preference_mqtt_broker_port);
    if(port == 0)
    {
        port = 1883;
        _preferences->putInt(preference_mqtt_broker_port, port);
    }

    String mqttPath = _preferences->getString(preference_mqtt_path);
    if(mqttPath.length() > 0)
    {
        size_t len = mqttPath.length();
        for(int i=0; i < len; i++)
        {
            _mqttPath[i] = mqttPath.charAt(i);
        }
    }
    else
    {
        strcpy(_mqttPath, "blescanner");
        _preferences->putString(preference_mqtt_path, _mqttPath);
    }

    String mqttUser = _preferences->getString(preference_mqtt_user);
    if(mqttUser.length() > 0)
    {
        size_t len = mqttUser.length();
        for(int i=0; i < len; i++)
        {
            _mqttUser[i] = mqttUser.charAt(i);
        }
    }

    String mqttPass = _preferences->getString(preference_mqtt_password);
    if(mqttPass.length() > 0)
    {
        size_t len = mqttPass.length();
        for(int i=0; i < len; i++)
        {
            _mqttPass[i] = mqttPass.charAt(i);
        }
    }

    Serial.print(F("MQTT Broker: "));
    Serial.print(_mqttBrokerAddr);
    Serial.print(F(":"));
    Serial.println(port);
    _mqttClient.setServer(_mqttBrokerAddr, port);
}


bool Network::reconnect()
{
    while (!_mqttClient.connected() && millis() > _nextReconnect)
    {
        Serial.println(F("Attempting MQTT connection"));
        bool success = false;

        if(strlen(_mqttUser) == 0)
        {
            Serial.println(F("MQTT: Connecting without credentials"));
            success = _mqttClient.connect("blescanner");
        }
        else
        {
            Serial.print(F("MQTT: Connecting with user: ")); Serial.println(_mqttUser);
            success = _mqttClient.connect("blescanner", _mqttUser, _mqttPass);
        }


        if (success) {
            Serial.println(F("MQTT connected"));
            _mqttConnected = true;
            delay(200);
        }
        else
        {
            Serial.print(F("MQTT connect failed, rc="));
            Serial.println(_mqttClient.state());
            _mqttConnected = false;
            _nextReconnect = millis() + 5000;
        }
    }
    return _mqttConnected;
}

void Network::update()
{
    if(!WiFi.isConnected())
    {
        Serial.println(F("WiFi not connected"));
        vTaskDelay( 1000 / portTICK_PERIOD_MS);
    }

    if(!_mqttClient.connected())
    {
        bool success = reconnect();
        if(!success)
        {
            return;
        }
    }

    if(_presenceCsv != nullptr && strlen(_presenceCsv) > 0)
    {
        publishString(mqtt_topic_presence, _presenceCsv);
        _presenceCsv = nullptr;
    }

    _mqttClient.loop();
}

void Network::publishPresenceDetection(char *csv)
{
    _presenceCsv = csv;
//    Serial.println(_presenceCsv);
}

void Network::publishFloat(const char* topic, const float value, const uint8_t precision)
{
    char str[30];
    dtostrf(value, 0, precision, str);
    char path[200] = {0};
    buildMqttPath(topic, path);
    _mqttClient.publish(path, str);
}

void Network::publishInt(const char *topic, const int value)
{

    char str[30];
    itoa(value, str, 10);
    char path[200] = {0};
    buildMqttPath(topic, path);
    _mqttClient.publish(path, str);
}

void Network::publishBool(const char *topic, const bool value)
{
    char str[2] = {0};
    str[0] = value ? '1' : '0';
    char path[200] = {0};
    buildMqttPath(topic, path);
    _mqttClient.publish(path, str);
}

void Network::publishString(const char *topic, const char *value)
{
    char path[200] = {0};
    buildMqttPath(topic, path);
    _mqttClient.publish(path, value);
}


bool Network::isMqttConnected()
{
    return _mqttConnected;
}

void Network::buildMqttPath(const char* path, char* outPath)
{
    int offset = 0;
    for(const char& c : _mqttPath)
    {
        if(c == 0x00)
        {
            break;
        }
        outPath[offset] = c;
        ++offset;
    }
    int i=0;
    while(outPath[i] != 0x00)
    {
        outPath[offset] = path[i];
        ++i;
        ++offset;
    }
    outPath[i+1] = 0x00;
}
