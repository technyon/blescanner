#pragma once


#include "Network.h"

class Gpio : public MqttReceiver
{
public:
    Gpio(Network* network);

    virtual void onMqttDataReceived(const char* topic, byte* payload, const unsigned int length) override;

private:
    static void IRAM_ATTR input_a();
    static void IRAM_ATTR input_b();
    static void IRAM_ATTR input_c();

    static Network* _network;
};
