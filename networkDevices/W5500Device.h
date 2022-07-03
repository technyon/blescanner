#pragma once

#include "NetworkDevice.h"
#include <Ethernet.h>
#include <Preferences.h>

class W5500Device : public NetworkDevice
{
public:
    explicit W5500Device(const String& hostname, Preferences* _preferences);
    ~W5500Device();

    virtual void initialize();
    virtual bool reconnect();
    virtual void reconfigure();
    virtual void printError();

    virtual void update();

    virtual bool isConnected();

    virtual PubSubClient *mqttClient();

private:
    void resetDevice();
    void initializeMacAddress(byte* mac);

    EthernetClient* _ethClient = nullptr;
    PubSubClient* _mqttClient = nullptr;
    Preferences* _preferences = nullptr;

    int _maintainResult = 0;
    bool _hasDHCPAddress = false;
    bool _restartOnDisconnect = false;

    byte _mac[6];
};