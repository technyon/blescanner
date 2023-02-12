#include <esp32-hal.h>
#include "Gpio.h"
#include "Arduino.h"
#include "Pins.h"
#include "MqttTopics.h"
#include "Logger.h"

Network* Gpio::_network = nullptr;

Gpio::Gpio(Network* network)
{
    _network = network;

    pinMode(INPUT_PIN_A, INPUT_PULLUP);
    pinMode(INPUT_PIN_B, INPUT_PULLUP);
    pinMode(INPUT_PIN_C, INPUT_PULLUP);
    pinMode(OUTPUT_PIN_A, OUTPUT);
    pinMode(OUTPUT_PIN_B, OUTPUT);
    pinMode(OUTPUT_PIN_C, OUTPUT);
    pinMode(OUTPUT_PIN_D, OUTPUT);
    pinMode(OUTPUT_PIN_E, OUTPUT);
    pinMode(OUTPUT_PIN_F, OUTPUT);

    attachInterrupt(INPUT_PIN_A, input_a, CHANGE);
    attachInterrupt(INPUT_PIN_B, input_b, CHANGE);
    attachInterrupt(INPUT_PIN_C, input_c, CHANGE);

    _network->publishPin(mqtt_topic_input_pin_a, 0);
    _network->publishPin(mqtt_topic_input_pin_b, 0);
    _network->publishPin(mqtt_topic_input_pin_c, 0);

    _network->subscribe(mqtt_topic_output_pin_a);
    _network->subscribe(mqtt_topic_output_pin_b);
    _network->subscribe(mqtt_topic_output_pin_c);
    _network->subscribe(mqtt_topic_output_pin_d);
    _network->subscribe(mqtt_topic_output_pin_e);
    _network->subscribe(mqtt_topic_output_pin_f);

    _network->registerMqttReceiver(this);
}

void Gpio::onMqttDataReceived(const char* topic, byte* payload, const unsigned int length)
{
    char* value = (char*)payload;

    if(_network->comparePrefixedPath(topic, mqtt_topic_output_pin_a))
    {
        digitalWrite(OUTPUT_PIN_A, strcmp(value, "1") == 0 ? HIGH : LOW);
    }
    else if(_network->comparePrefixedPath(topic, mqtt_topic_output_pin_b))
    {
        digitalWrite(OUTPUT_PIN_B, strcmp(value, "1") == 0 ? HIGH : LOW);
    }
    else if(_network->comparePrefixedPath(topic, mqtt_topic_output_pin_c))
    {
        digitalWrite(OUTPUT_PIN_C, strcmp(value, "1") == 0 ? HIGH : LOW);
    }
    else if(_network->comparePrefixedPath(topic, mqtt_topic_output_pin_d))
    {
        digitalWrite(OUTPUT_PIN_D, strcmp(value, "1") == 0 ? HIGH : LOW);
    }
    else if(_network->comparePrefixedPath(topic, mqtt_topic_output_pin_e))
    {
        digitalWrite(OUTPUT_PIN_E, strcmp(value, "1") == 0 ? HIGH : LOW);
    }
    else if(_network->comparePrefixedPath(topic, mqtt_topic_output_pin_f))
    {
        Serial.println(value);
        Serial.println(strcmp(value, "1") == 0 ? "HIGH" : "LOW");
        digitalWrite(OUTPUT_PIN_F, strcmp(value, "1") == 0 ? HIGH : LOW);
    }
}

void Gpio::input_a()
{
    _network->publishPin(mqtt_topic_input_pin_a, digitalRead(INPUT_PIN_A) == 0 ? 1 : 0);
}

void Gpio::input_b()
{
    _network->publishPin(mqtt_topic_input_pin_b, digitalRead(INPUT_PIN_B) == 0 ? 1 : 0);
}

void Gpio::input_c()
{
    _network->publishPin(mqtt_topic_input_pin_c, digitalRead(INPUT_PIN_C) == 0 ? 1 : 0);
}
