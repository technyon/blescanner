#include "Arduino.h"
#include "Network.h"
#include "WebCfgServer.h"
#include <FreeRTOS.h>
#include "PreferencesKeys.h"
#include "PresenceDetection.h"
#include "BleScanner.h"

#define ESP32

Network* network;
WebCfgServer* webCfgServer;
PresenceDetection* presenceDetection;
Preferences* preferences;
BleScanner* bleScanner;

void networkTask(void *pvParameters)
{
    while(true)
    {
        network->update();
        webCfgServer->update();
        vTaskDelay( 100 / portTICK_PERIOD_MS);
    }
}

void presenceDetectionTask(void *pvParameters)
{
    while(true)
    {
        presenceDetection->update();
    }
}

void bleScannerTask(void *pvParameters)
{
    while(true)
    {
        bleScanner->update();
        vTaskDelay( 20 / portTICK_PERIOD_MS);
    }
}

void checkMillisTask(void *pvParameters)
{
    while(true)
    {
        vTaskDelay( 60000 / portTICK_PERIOD_MS);
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

    xTaskCreatePinnedToCore(networkTask, "ntw", 65536, nullptr, 3, nullptr, 1);
    xTaskCreatePinnedToCore(presenceDetectionTask, "prdet", 16384, nullptr, 5, nullptr, 1);
    xTaskCreatePinnedToCore(bleScannerTask, "blescan", 16384, nullptr, 1, nullptr, 1);
    xTaskCreatePinnedToCore(checkMillisTask, "millis", 512, nullptr, 1, nullptr, 1);
}

void setup()
{
    Serial.begin(115200);

    preferences = new Preferences();
    preferences->begin("blescanner", false);
    network = new Network(preferences);
    network->initialize();

    webCfgServer = new WebCfgServer(network, preferences);
    webCfgServer->initialize();

    bleScanner = new BleScanner();
    bleScanner->initialize();
    bleScanner->setScanDuration(5);

    presenceDetection = new PresenceDetection(preferences, bleScanner, network);
    presenceDetection->initialize();

    setupTasks();
}

void loop()
{}