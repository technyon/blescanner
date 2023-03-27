#include "IPConfiguration.h"
#include "../PreferencesKeys.h"
#include "../Logger.h"

IPConfiguration::IPConfiguration()
{
}

bool IPConfiguration::dhcpEnabled() const
{
    return true;
}

const IPAddress IPConfiguration::ipAddress() const
{
    return IPAddress();
}

const IPAddress IPConfiguration::subnet() const
{
    return IPAddress();
}

const IPAddress IPConfiguration::defaultGateway() const
{
    return IPAddress();
}

const IPAddress IPConfiguration::dnsServer() const
{
    return IPAddress();
}
