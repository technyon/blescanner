#pragma once


#include "Network.h"

class Gpio : public MqttReceiver
{
public:
    Gpio(Network* network);

    virtual void onMqttDataReceived(char *&topic, byte *&payload, unsigned int &length);

private:
    static void IRAM_ATTR input_a();
    static void IRAM_ATTR input_b();
    static void IRAM_ATTR input_c();

    static Network* _network;
};
