#include "espMqttClientW5500.h"

espMqttClientW5500::espMqttClientW5500(uint8_t priority, uint8_t core)
: MqttClientSetup(true, priority, core),
  _client()
{
    _transport = &_client;
}
