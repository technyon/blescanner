#pragma once

#include <Preferences.h>

class IPConfiguration
{
public:
    explicit IPConfiguration();

    bool dhcpEnabled() const;
    const IPAddress ipAddress() const;
    const IPAddress subnet() const;
    const IPAddress defaultGateway() const;
    const IPAddress dnsServer() const;
};

