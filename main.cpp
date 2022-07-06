#include "Arduino.h"
#include "Pins.h"
#include "WebCfgServer.h"
#include <RTOS.h>
#include "PreferencesKeys.h"
#include "PresenceDetection.h"
#include "hardware/W5500EthServer.h"
#include "hardware/WifiEthServer.h"
#include "Gpio.h"

Network* network = nullptr;
Gpio* gpio = nullptr;
WebCfgServer* webCfgServer = nullptr;
BleScanner::Scanner* bleScanner = nullptr;
PresenceDetection* presenceDetection = nullptr;
Preferences* preferences = nullptr;
EthServer* ethServer = nullptr;

bool lockEnabled = false;
bool openerEnabled = false;

void networkTask(void *pvParameters)
{
    while(true)
    {
        int r = network->update();

        switch(r)
        {
            // Network Device and MQTT is connected. Process all updates.
            case 0:
                network->update();
                webCfgServer->update();
                break;
            case 1:
                // Network Device is connected, but MQTT isn't. Call network->update() to allow MQTT reconnect and
                // keep Webserver alive to allow user to reconfigure network settings
                network->update();
                webCfgServer->update();
                break;
                // Neither Network Devicce or MQTT is connected
            default:
                network->update();
                break;
        }

        delay(50);
    }
}

void bleScannerTask(void *pvParameters)
{
    while(true)
    {
        bleScanner->update();
        delay(20);
    }
}

void presenceDetectionTask(void *pvParameters)
{
    while(true)
    {
        presenceDetection->update();
    }
}

void checkMillisTask(void *pvParameters)
{
    while(true)
    {
        delay(60000);
        // millis() is about to overflow. Restart device to prevent problems with overflow
        if(millis() > (2^32) - 5 * 60000)
        {
            Serial.println(F("millis() is about to overflow. Restarting device."));
            vTaskDelay( 2000 / portTICK_PERIOD_MS);
            ESP.restart();
        }
    }
}


void setupTasks()
{
    // configMAX_PRIORITIES is 25

    xTaskCreatePinnedToCore(networkTask, "ntw", 8192, NULL, 3, NULL, 1);
    xTaskCreatePinnedToCore(bleScannerTask, "scan", 4096, NULL, 2, NULL, 1);
    xTaskCreatePinnedToCore(presenceDetectionTask, "prdet", 768, NULL, 5, NULL, 1);
    xTaskCreatePinnedToCore(checkMillisTask, "mlchk", 640, NULL, 1, NULL, 1);
}

uint32_t getRandomId()
{
    uint8_t rnd[4];
    for(int i=0; i<4; i++)
    {
        rnd[i] = random(255);
    }
    uint32_t deviceId;
    memcpy(&deviceId, &rnd, sizeof(deviceId));
    return deviceId;
}

void initEthServer(const NetworkDeviceType device)
{
    switch (device)
    {
        case NetworkDeviceType::W5500:
            ethServer = new W5500EthServer(80);
            break;
        case NetworkDeviceType::WiFi:
            ethServer = new WifiEthServer(80);
            break;
        default:
            ethServer = new WifiEthServer(80);
            break;
    }
}

void setup()
{
    pinMode(NETWORK_SELECT, INPUT_PULLUP);

    Serial.begin(115200);

    preferences = new Preferences();
    preferences->begin("blescanner", false);

//    const NetworkDeviceType networkDevice = NetworkDeviceType::WiFi;
    const NetworkDeviceType networkDevice = digitalRead(NETWORK_SELECT) == HIGH ? NetworkDeviceType::WiFi : NetworkDeviceType::W5500;

    network = new Network(networkDevice, preferences);
    network->initialize();

    if(preferences->getBool(preference_gpio_enabled))
    {
        gpio = new Gpio(network);
    }

    uint32_t deviceId = preferences->getUInt(preference_deviceId);
    if(deviceId == 0)
    {
        deviceId = getRandomId();
        preferences->putUInt(preference_deviceId, deviceId);
    }

    initEthServer(networkDevice);

    bleScanner = new BleScanner::Scanner();
    bleScanner->initialize("blescanner");
    bleScanner->setScanDuration(10);

    webCfgServer = new WebCfgServer(network, ethServer, preferences, networkDevice == NetworkDeviceType::WiFi);
    webCfgServer->initialize();

    presenceDetection = new PresenceDetection(preferences, bleScanner, network);
    presenceDetection->initialize();

    setupTasks();
}

void loop()
{}