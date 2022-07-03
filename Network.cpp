#include "Network.h"
#include "PreferencesKeys.h"
#include "MqttTopics.h"
#include "networkDevices/W5500Device.h"
#include "networkDevices/WifiDevice.h"


Network* Network::_inst = nullptr;

Network::Network(const NetworkDeviceType networkDevice, Preferences *preferences)
: _preferences(preferences)
{
    _inst = this;
    _hostname = _preferences->getString(preference_hostname);
    setupDevice(networkDevice);
}


void Network::setupDevice(const NetworkDeviceType hardware)
{
    switch(hardware)
    {
        case NetworkDeviceType::W5500:
            Serial.println(F("Network device: W5500"));
            _device = new W5500Device(_hostname, _preferences);
            break;
        case NetworkDeviceType::WiFi:
            Serial.println(F("Network device: Builtin WiFi"));
            _device = new WifiDevice(_hostname, _preferences);
            break;
        default:
            Serial.println(F("Unknown network device type, defaulting to WiFi"));
            _device = new WifiDevice(_hostname, _preferences);
            break;
    }
}

void Network::initialize()
{
    if(_hostname == "")
    {
        _hostname = "blescanner";
        _preferences->putString(preference_hostname, _hostname);
    }

    String path = _preferences->getString(preference_mqtt_path);
    if(path == "")
    {
        path = "blescanner";
        _preferences->putString(preference_mqtt_path, path);
    }

    strcpy(_mqttPath, path.c_str());

    _device->initialize();

    Serial.print(F("Host name: "));
    Serial.println(_hostname);

    const char* brokerAddr = _preferences->getString(preference_mqtt_broker).c_str();
    strcpy(_mqttBrokerAddr, brokerAddr);

    int port = _preferences->getInt(preference_mqtt_broker_port);
    if(port == 0)
    {
        port = 1883;
        _preferences->putInt(preference_mqtt_broker_port, port);
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

    _device->mqttClient()->setServer(_mqttBrokerAddr, port);
    _device->mqttClient()->setCallback(Network::onMqttDataReceivedCallback);

    _networkTimeout = _preferences->getInt(preference_network_timeout);
    if(_networkTimeout == 0)
    {
        _networkTimeout = -1;
        _preferences->putInt(preference_network_timeout, _networkTimeout);
    }
}

bool Network::update()
{
    long ts = millis();

    _device->update();

    if(!_device->isConnected())
    {
        Serial.println(F("Network not connected. Trying reconnect."));
        bool success = _device->reconnect();
        Serial.println(success ? F("Reconnect successful") : F("Reconnect failed"));
    }

    if(!_device->isConnected())
    {
        if(_networkTimeout > 0 && (ts - _lastConnectedTs > _networkTimeout * 1000))
        {
            Serial.println("Network timeout has been reached, restarting ...");
            delay(200);
            ESP.restart();
        }
        return false;
    }

    _lastConnectedTs = ts;

    if(!_device->mqttClient()->connected())
    {
        bool success = reconnect();
        if(!success)
        {
            return false;
        }
    }

    if(_presenceCsv != nullptr && strlen(_presenceCsv) > 0)
    {
        bool success = publishString(mqtt_topic_presence, _presenceCsv);
        if(!success)
        {
            Serial.println(F("Failed to publish presence CSV data."));
            Serial.println(_presenceCsv);
        }
        _presenceCsv = nullptr;
    }

    for(const auto& pin : _pinStates)
    {
        if(pin.second != -1)
        {
            publishInt(pin.first, pin.second);
        }
        _pinStates[pin.first] = -1;
    }

    _device->mqttClient()->loop();
    return true;
}

bool Network::reconnect()
{
    _mqttConnected = false;

    while (!_device->mqttClient()->connected() && millis() > _nextReconnect)
    {
        Serial.println(F("Attempting MQTT connection"));
        bool success = false;

        if(strlen(_mqttUser) == 0)
        {
            Serial.println(F("MQTT: Connecting without credentials"));
            success = _device->mqttClient()->connect(_preferences->getString(preference_hostname).c_str());
        }
        else
        {
            Serial.print(F("MQTT: Connecting with user: ")); Serial.println(_mqttUser);
            success = _device->mqttClient()->connect(_preferences->getString(preference_hostname).c_str(), _mqttUser, _mqttPass);
        }

        if (success)
        {
            Serial.println(F("MQTT connected"));
            _mqttConnected = true;
            delay(100);
            for(const String& topic : _subscribedTopics)
            {
                _device->mqttClient()->subscribe(topic.c_str());
            }
        }
        else
        {
            Serial.print(F("MQTT connect failed, rc="));
            Serial.println(_device->mqttClient()->state());
            _device->printError();
            _device->mqttClient()->disconnect();
            _mqttConnected = false;
            _nextReconnect = millis() + 5000;
        }
    }
    return _mqttConnected;
}

void Network::subscribe(const char *path)
{
    char prefixedPath[500];
    buildMqttPath(path, prefixedPath);
    _subscribedTopics.push_back(prefixedPath);
}

void Network::buildMqttPath(const char* path, char* outPath)
{
    int offset = 0;
    int i=0;
    while(_mqttPath[i] != 0x00)
    {
        outPath[offset] = _mqttPath[i];
        ++offset;
        ++i;
    }

    i=0;
    while(outPath[i] != 0x00)
    {
        outPath[offset] = path[i];
        ++i;
        ++offset;
    }

    outPath[i+1] = 0x00;
}

void Network::registerMqttReceiver(MqttReceiver* receiver)
{
    _mqttReceivers.push_back(receiver);
}

void Network::onMqttDataReceivedCallback(char *topic, byte *payload, unsigned int length)
{
    _inst->onMqttDataReceived(topic, payload, length);
}

void Network::onMqttDataReceived(char *&topic, byte *&payload, unsigned int &length)
{
    for(auto receiver : _mqttReceivers)
    {
        receiver->onMqttDataReceived(topic, payload, length);
    }
}

PubSubClient *Network::mqttClient()
{
    return _device->mqttClient();
}

void Network::reconfigureDevice()
{
    _device->reconfigure();
}

bool Network::isMqttConnected()
{
    return _mqttConnected;
}


void Network::publishFloat(const char* topic, const float value, const uint8_t precision)
{
    char str[30];
    dtostrf(value, 0, precision, str);
    char path[200] = {0};
    buildMqttPath(topic, path);
    _device->mqttClient()->publish(path, str, true);
}

void Network::publishInt(const char *topic, const int value)
{
    char str[30];
    itoa(value, str, 10);
    char path[200] = {0};
    buildMqttPath(topic, path);
    _device->mqttClient()->publish(path, str, true);
}

void Network::publishUInt(const char *topic, const unsigned int value)
{
    char str[30];
    utoa(value, str, 10);
    char path[200] = {0};
    buildMqttPath(topic, path);
    _device->mqttClient()->publish(path, str, true);
}

void Network::publishBool(const char *topic, const bool value)
{
    char str[2] = {0};
    str[0] = value ? '1' : '0';
    char path[200] = {0};
    buildMqttPath(topic, path);
    _device->mqttClient()->publish(path, str, true);
}

bool Network::publishString(const char *topic, const char *value)
{
    char path[200] = {0};
    buildMqttPath(topic, path);
    return _device->mqttClient()->publish(path, value, true);
}

void Network::publishPin(const char *topic, int value)
{
    _pinStates[topic] = value;
}

bool Network::comparePrefixedPath(const char *fullPath, const char *subPath)
{
    char prefixedPath[500];
    buildMqttPath(subPath, prefixedPath);
    return strcmp(fullPath, prefixedPath) == 0;
}

void Network::publishPresenceDetection(char *csv)
{
    _presenceCsv = csv;
}
