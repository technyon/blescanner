#pragma once

#include <Arduino.h>

class MqttReceiver
{
public:
    virtual void onMqttDataReceived(const char* topic, byte* payload, const unsigned int length) = 0;
};