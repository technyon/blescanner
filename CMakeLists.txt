cmake_minimum_required(VERSION 3.0.0)

set(ARDUINO_BOARD "ESP32 Dev Module [esp32.esp32]")

project(blescanner CXX)

# ARDUHAL_LOG_LEVEL_NONE, define ARDUHAL_LOG_LEVEL_ERROR, define ARDUHAL_LOG_LEVEL_WARN, define ARDUHAL_LOG_LEVEL_INFO,
# define ARDUHAL_LOG_LEVEL_DEBUG, define ARDUHAL_LOG_LEVEL_VERBOSE

set(LOG_LEVEL ARDUHAL_LOG_LEVEL_NONE)

# Length of char arrays to store certificates for MQTTS
add_compile_definitions(TLS_CA_MAX_SIZE=1500)
add_compile_definitions(TLS_CERT_MAX_SIZE=1500)
add_compile_definitions(TLS_KEY_MAX_SIZE=1800)
add_compile_definitions(ESP_PLATFORM)
add_compile_definitions(ESP32)

include_directories(${PROJECT_NAME}
        PRIVATE
        lib/Crc16
        lib/NimBLE-Arduino/src
        lib/BleScanner/src
        lib/WiFiManager
        lib/WebServer/src
        lib/Ethernet/src
        lib/espMqttClient/src
        lib/AsyncTCP/src
        )

set(SRCFILES
        Pins.h
        Network.cpp
        MqttReceiver.h
        networkDevices/NetworkDevice.h
        networkDevices/WifiDevice.cpp
        networkDevices/W5500Device.cpp
        networkDevices/ClientSyncW5500.cpp
        networkDevices/espMqttClientW5500.cpp
        networkDevices/IPConfiguration.cpp
        MqttTopics.h
        Logger.cpp
        RestartReason.h
        Ota.cpp
        WebCfgServerConstants.h
        WebCfgServer.cpp
        PresenceDetection.cpp
        PreferencesKeys.h
        SpiffsCookie.cpp
        Gpio.cpp
        Version.h
        lib/WiFiManager/WiFiManager.cpp
        lib/Crc16/Crc16.h
        lib/BleScanner/src/BleInterfaces.h
        lib/BleScanner/src/BleScanner.cpp
        lib/AsyncTCP/src/AsyncTCP.cpp
        )

file(GLOB_RECURSE SRCFILESREC
        lib/NimBLE-Arduino/src/*.c
        lib/NimBLE-Arduino/src/*.cpp
        lib/NimBLE-Arduino/src/*.h
        lib/WebServer/src/*.cpp
        lib/WebServer/src/*.h
        lib/Ethernet/src/*.cpp
        lib/Ethernet/src/*.h
        lib/espMqttClient/src/*.cpp
        lib/espMqttClient/src/*.h
        lib/espMqttClient/src/Packets/*.cpp
        lib/espMqttClient/src/Packets/*.h
        lib/espMqttClient/src/Transport/*.cpp
        lib/espMqttClient/src/Transport/*.h
        )

add_executable(${PROJECT_NAME}
        main.cpp
        ${SRCFILES}
        ${SRCFILESREC}
        )

target_compile_definitions(${PROJECT_NAME}
        PRIVATE
        ARDUHAL_LOG_LEVEL=${LOG_LEVEL}
        CORE_DEBUG_LEVEL=${LOG_LEVEL}
        CONFIG_NIMBLE_CPP_LOG_LEVEL=0
        )

target_link_arduino_libraries(${PROJECT_NAME}
    PRIVATE
        core
        BLE
        WiFi
        WiFiClientSecure
        Update
#        WebServer
        DNSServer
        Preferences
        SPIFFS
        SPI
#        esp32
#        Wire
#        FS
)

target_link_arduino_libraries(DNSServer PUBLIC WiFi)

# This is needed for the generation of HEX binary and uploading it
target_enable_arduino_upload(${PROJECT_NAME})

