#include "WebCfgServer.h"
#include "PreferencesKeys.h"
#include "Version.h"
#include "hardware/WifiEthServer.h"
#include <esp_task_wdt.h>

WebCfgServer::WebCfgServer(Network* network, EthServer* ethServer, Preferences* preferences, bool allowRestartToPortal)
: _server(ethServer),
  _network(network),
  _preferences(preferences),
  _allowRestartToPortal(allowRestartToPortal)
{
    String str = _preferences->getString(preference_cred_user);
    _hostname = _preferences->getString(preference_hostname);

    if(str.length() > 0)
    {
        memset(&_credUser, 0, sizeof(_credUser));
        memset(&_credPassword, 0, sizeof(_credPassword));

        _hasCredentials = true;
        const char *user = str.c_str();
        memcpy(&_credUser, user, str.length());

        str = _preferences->getString(preference_cred_password);
        const char *pass = str.c_str();
        memcpy(&_credPassword, pass, str.length());
    }
}

void WebCfgServer::initialize()
{
    _server.on("/", [&]() {
        Serial.print("Expected user    : "); Serial.println(_credUser);
        Serial.print("Expected password: "); Serial.println(_credPassword);
        if (_hasCredentials && !_server.authenticate(_credUser, _credPassword)) {
            return _server.requestAuthentication();
        }
        String response = "";
        buildHtml(response);
        _server.send(200, "text/html", response);
    });
    _server.on("/new.css", [&]() {
        if (_hasCredentials && !_server.authenticate(_credUser, _credPassword)) {
            return _server.requestAuthentication();
        }
        sendNewCss();
    });
    _server.on("/inter.css", [&]() {
        if (_hasCredentials && !_server.authenticate(_credUser, _credPassword)) {
            return _server.requestAuthentication();
        }
        sendFontsInterMinCss();
    });
    _server.on("/cred", [&]() {
        if (_hasCredentials && !_server.authenticate(_credUser, _credPassword)) {
            return _server.requestAuthentication();
        }
        String response = "";
        buildCredHtml(response);
        _server.send(200, "text/html", response);
    });
    _server.on("/mqttconfig", [&]() {
        if (_hasCredentials && !_server.authenticate(_credUser, _credPassword)) {
            return _server.requestAuthentication();
        }
        String response = "";
        buildMqttConfigHtml(response);
        _server.send(200, "text/html", response);
    });
    _server.on("/wifi", [&]() {
        if (_hasCredentials && !_server.authenticate(_credUser, _credPassword)) {
            return _server.requestAuthentication();
        }
        String response = "";
        buildConfigureWifiHtml(response);
        _server.send(200, "text/html", response);
    });
    _server.on("/wifimanager", [&]() {
        if (_hasCredentials && !_server.authenticate(_credUser, _credPassword)) {
            return _server.requestAuthentication();
        }
        if(_allowRestartToPortal)
        {
            String response = "";
            buildConfirmHtml(response, "Restarting. Connect to ESP access point to reconfigure WiFi.", 0);
            _server.send(200, "text/html", response);
            waitAndProcess(true, 2000);
            _network->reconfigureDevice();
        }
    });
    _server.on("/restart", [&]() {
        if (_hasCredentials && !_server.authenticate(_credUser, _credPassword)) {
            return _server.requestAuthentication();
        }
        String response = "";
        buildConfirmHtml(response, "Restarting device, please wait.", 5);
        _server.send(200, "text/html", response);
        Serial.println(F("Restarting"));

        waitAndProcess(true, 1000);
        ESP.restart();
    });

    _server.on("/method=get", [&]() {
        if (_hasCredentials && !_server.authenticate(_credUser, _credPassword)) {
            return _server.requestAuthentication();
        }
        String message = "";
        bool restartEsp = processArgs(message);
        if(restartEsp)
        {
            String response = "";
            buildConfirmHtml(response, message);
            _server.send(200, "text/html", response);
            Serial.println(F("Restarting"));

            waitAndProcess(true, 1000);
            ESP.restart();
        }
        else
        {
            String response = "";
            buildConfirmHtml(response, message, 3);
            _server.send(200, "text/html", response);
            waitAndProcess(false, 1000);
        }
    });
    _server.on("/ota", [&]() {
        if (_hasCredentials && !_server.authenticate(_credUser, _credPassword)) {
            return _server.requestAuthentication();
        }
        String response = "";
        buildOtaHtml(response);
        _server.send(200, "text/html", response);
    });
    _server.on("/uploadota", HTTP_POST, [&]() {
        if (_hasCredentials && !_server.authenticate(_credUser, _credPassword)) {
            return _server.requestAuthentication();
        }

        _server.send(200, "text/html", "");
    }, [&]() {
        if (_hasCredentials && !_server.authenticate(_credUser, _credPassword)) {
            return _server.requestAuthentication();
        }

        handleOtaUpload();
    });

    _server.begin();
}

bool WebCfgServer::processArgs(String& message)
{
    bool configChanged = false;
    bool clearMqttCredentials = false;
    bool clearCredentials = false;

    int count = _server.args();
    for(int index = 0; index < count; index++)
    {
        String key = _server.argName(index);
        String value = _server.arg(index);

        if(key == "MQTTSERVER")
        {
            _preferences->putString(preference_mqtt_broker, value);
            configChanged = true;
        }
        else if(key == "MQTTPORT")
        {
            _preferences->putInt(preference_mqtt_broker_port, value.toInt());
            configChanged = true;
        }
        else if(key == "MQTTUSER")
        {
            if(value == "#")
            {
                clearMqttCredentials = true;
            }
            else
            {
                _preferences->putString(preference_mqtt_user, value);
                configChanged = true;
            }
        }
        else if(key == "MQTTPASS")
        {
            if(value != "*")
            {
                _preferences->putString(preference_mqtt_password, value);
                configChanged = true;
            }
        }
        else if(key == "MQTTPATH")
        {
            _preferences->putString(preference_mqtt_path, value);
            configChanged = true;
        }
        else if(key == "MQTTCA")
        {
            _preferences->putString(preference_mqtt_ca, value);
            configChanged = true;
        }
        else if(key == "MQTTCRT")
        {
            _preferences->putString(preference_mqtt_crt, value);
            configChanged = true;
        }
        else if(key == "MQTTKEY")
        {
            _preferences->putString(preference_mqtt_key, value);
            configChanged = true;
        }
        else if(key == "HOSTNAME")
        {
            _preferences->putString(preference_hostname, value);
            configChanged = true;
        }
        else if(key == "NETTIMEOUT")
        {
            _preferences->putInt(preference_network_timeout, value.toInt());
            configChanged = true;
        }
        else if(key == "RSTDISC")
        {
            _preferences->putBool(preference_restart_on_disconnect, (value == "1"));
            configChanged = true;
        }
        else if(key == "PRDTMO")
        {
            _preferences->putInt(preference_presence_detection_timeout, value.toInt());
            configChanged = true;
        }
        else if(key == "GPLCK")
        {
            _preferences->putBool(preference_gpio_enabled, (value == "1"));
            configChanged = true;
        }
        else if(key == "CREDUSER")
        {
            if(value == "#")
            {
                clearCredentials = true;
            }
            else
            {
                _preferences->putString(preference_cred_user, value);
                configChanged = true;
            }
        }
        else if(key == "CREDPASS")
        {
            _preferences->putString(preference_cred_password, value);
            configChanged = true;
        }
    }

    if(clearMqttCredentials)
    {
        _preferences->putString(preference_mqtt_user, "");
        _preferences->putString(preference_mqtt_password, "");
        configChanged = true;
    }

    if(clearCredentials)
    {
        _preferences->putString(preference_cred_user, "");
        _preferences->putString(preference_cred_password, "");
        configChanged = true;
    }

    if(configChanged)
    {
        message = "Configuration saved ... restarting.";
        _enabled = false;
        _preferences->end();
    }

    return configChanged;
}

void WebCfgServer::update()
{
    if(!_enabled) return;

    _server.handleClient();
}

void WebCfgServer::buildHtml(String& response)
{
    buildHtmlHeader(response);

    response.concat("<br><h3>Info</h3>\n");

    String version = blescanner_version;

    response.concat("<table>");

    printParameter(response, "MQTT Connected", _network->isMqttConnected() ? "Yes" : "No");

    printParameter(response, "Firmware", version.c_str());
    response.concat("</table><br><br>");

    response.concat("<h3>Device Configuration</h3>");
    buildNavigationButton(response, "Edit", "/mqttconfig");

    response.concat("<BR><BR><h3>Credentials</h3>");
    buildNavigationButton(response, "Edit", "/cred");

    response.concat("<BR><BR><h3>Firmware update</h3>");
    buildNavigationButton(response, "Open", "/ota");

    if(_allowRestartToPortal)
    {
        response.concat("<br><br><h3>WiFi</h3>");
        buildNavigationButton(response, "Restart and configure wifi", "/wifi");
    }

    response.concat("<BR><BR><h3>Restart device</h3>");
    buildNavigationButton(response, "Restart", "/restart");

    response.concat("</BODY></HTML>");
}


void WebCfgServer::buildCredHtml(String &response)
{
    buildHtmlHeader(response);

    response.concat("<FORM ACTION=method=get >");
    response.concat("<h3>Credentials</h3>");
    response.concat("<table>");
    printInputField(response, "CREDUSER", "User (# to clear)", _preferences->getString(preference_cred_user).c_str(), 20);
    printInputField(response, "CREDPASS", "Password", "*", 30, true);
    response.concat("</table>");
    response.concat("<br><INPUT TYPE=SUBMIT NAME=\"submit\" VALUE=\"Save\">");
    response.concat("</FORM>");
    response.concat("</BODY></HTML>");
}

void WebCfgServer::buildOtaHtml(String &response)
{
    buildHtmlHeader(response);

    if(millis() < 60000)
    {
        response.concat("OTA functionality not ready. Please wait a moment and reload.");
        response.concat("</BODY></HTML>");
        return;
    }

    response.concat("<form id=\"upform\" enctype=\"multipart/form-data\" action=\"/uploadota\" method=\"POST\"><input type=\"hidden\" name=\"MAX_FILE_SIZE\" value=\"100000\" />Choose the updated blescanner.bin file to upload: <input name=\"uploadedfile\" type=\"file\" accept=\".bin\" /><br/>");
    response.concat("<br><input id=\"submitbtn\" type=\"submit\" value=\"Upload File\" /></form>");
    response.concat("<div id=\"msgdiv\" style=\"visibility:hidden\">Initiating Over-the-air update. This will take about a minute, please be patient.<br>You will be forwarded automatically when the update is complete.</div>");
    response.concat("<script type=\"text/javascript\">");
    response.concat("window.addEventListener('load', function () {");
    response.concat("	var button = document.getElementById(\"submitbtn\");");
    response.concat("	button.addEventListener('click',hideshow,false);");
    response.concat("	function hideshow() {");
    response.concat("		document.getElementById('upform').style.visibility = 'hidden';");
    response.concat("		document.getElementById('msgdiv').style.visibility = 'visible';");
    response.concat("		setTimeout(\"location.href = '/';\",60000);");
    response.concat("	}");
    response.concat("});");
    response.concat("</script>");
    response.concat("</BODY></HTML>");
}

void WebCfgServer::buildMqttConfigHtml(String &response)
{
    buildHtmlHeader(response);
    response.concat("<FORM ACTION=method=get >");
    response.concat("<h3>MQTT Configuration</h3>");
    response.concat("<table>");
    printInputField(response, "HOSTNAME", "Host name", _preferences->getString(preference_hostname).c_str(), 100);
    printInputField(response, "MQTTSERVER", "MQTT Broker", _preferences->getString(preference_mqtt_broker).c_str(), 100);
    printInputField(response, "MQTTPORT", "MQTT Broker port", _preferences->getInt(preference_mqtt_broker_port), 5);
    printInputField(response, "MQTTPATH", "MQTT Path", _preferences->getString(preference_mqtt_path).c_str(), 180);
    printInputField(response, "MQTTUSER", "MQTT User (# to clear)", _preferences->getString(preference_mqtt_user).c_str(), 30);
    printInputField(response, "MQTTPASS", "MQTT Password", "*", 30, true);
    printTextarea(response, "MQTTCA", "MQTT SSL CA Certificate (*, optional)", _preferences->getString(preference_mqtt_ca).c_str(), TLS_CA_MAX_SIZE);
    printTextarea(response, "MQTTCRT", "MQTT SSL Client Certificate (*, optional)", _preferences->getString(preference_mqtt_crt).c_str(), TLS_CERT_MAX_SIZE);
    printTextarea(response, "MQTTKEY", "MQTT SSL Client Key (*, optional)", _preferences->getString(preference_mqtt_key).c_str(), TLS_KEY_MAX_SIZE);
    printCheckBox(response, "GPLCK", "Enable control via GPIO", _preferences->getBool(preference_gpio_enabled));
    printInputField(response, "PRDTMO", "Presence detection timeout (seconds; -1 to disable)", _preferences->getInt(preference_presence_detection_timeout), 10);
    printInputField(response, "NETTIMEOUT", "Network Timeout until restart (seconds; -1 to disable)", _preferences->getInt(preference_network_timeout), 5);
    printCheckBox(response, "RSTDISC", "Restart on disconnect", _preferences->getBool(preference_restart_on_disconnect));
    response.concat("</table>");
    response.concat("* If no encryption is configured for the MQTT broker, leave empty.<br>");

    response.concat("<br><INPUT TYPE=SUBMIT NAME=\"submit\" VALUE=\"Save\">");
    response.concat("</FORM>");
    response.concat("</BODY></HTML>");
}

void WebCfgServer::buildConfirmHtml(String &response, const String &message, uint32_t redirectDelay)
{
    String delay(redirectDelay);

    response.concat("<HTML>\n");
    response.concat("<HEAD>\n");
    response.concat("<TITLE>BLE Scanner - ");
    response.concat(_hostname);
    response.concat("</TITLE>\n");
    response.concat("<meta http-equiv=\"Refresh\" content=\"");
    response.concat(redirectDelay);
    response.concat("; url=/\" />");
    response.concat("\n</HEAD>\n");
    response.concat("<BODY>\n");
    response.concat(message);

    response.concat("</BODY></HTML>");
}

void WebCfgServer::buildConfigureWifiHtml(String &response)
{
    buildHtmlHeader(response);

    response.concat("<h3>WiFi</h3>");
    response.concat("Click confirm to restart ESP into WiFi configuration mode. After restart, connect to ESP access point to reconfigure WiFI.<br><br>");
    buildNavigationButton(response, "Confirm", "/wifimanager");

    response.concat("</BODY></HTML>");
}

void WebCfgServer::buildHtmlHeader(String &response)
{
    response.concat("<HTML><HEAD>");
    response.concat("<meta name='viewport' content='width=device-width, initial-scale=1'>");
    response.concat("<link rel='stylesheet' href='/inter.css'><link rel='stylesheet' href='/new.css'>");
    response.concat("<TITLE>BLE Scanner - ");
    response.concat(_hostname);
    response.concat("</TITLE></HEAD><BODY>");
}

void WebCfgServer::printInputField(String& response,
                                   const char *token,
                                   const char *description,
                                   const char *value,
                                   const size_t maxLength,
                                   const bool isPassword)
{
    char maxLengthStr[20];

    itoa(maxLength, maxLengthStr, 10);

    response.concat("<tr><td>");
    response.concat(description);
    response.concat("</td><td>");
    response.concat("<INPUT TYPE=");
    response.concat(isPassword ? "PASSWORD" : "TEXT");
    response.concat(" VALUE=\"");
    response.concat(value);
    response.concat("\" NAME=\"");
    response.concat(token);
    response.concat("\" SIZE=\"25\" MAXLENGTH=\"");
    response.concat(maxLengthStr);
    response.concat("\\\"/>");
    response.concat("</td></tr>\"");
}

void WebCfgServer::printInputField(String& response,
                                   const char *token,
                                   const char *description,
                                   const int value,
                                   size_t maxLength)
{
    char valueStr[20];
    itoa(value, valueStr, 10);
    printInputField(response, token, description, valueStr, maxLength);
}

void WebCfgServer::printCheckBox(String &response, const char *token, const char *description, const bool value)
{
    response.concat("<tr><td>");
    response.concat(description);
    response.concat("</td><td>");

    response.concat("<INPUT TYPE=hidden NAME=\"");
    response.concat(token);
    response.concat("\" value=\"0\"");
    response.concat("/>");

    response.concat("<INPUT TYPE=checkbox NAME=\"");
    response.concat(token);
    response.concat("\" value=\"1\"");
    response.concat(value ? " checked=\"checked\"" : "");
    response.concat("/></td></tr>");
}

void WebCfgServer::printTextarea(String& response,
                                   const char *token,
                                   const char *description,
                                   const char *value,
                                   const size_t maxLength)
{
    char maxLengthStr[20];

    itoa(maxLength, maxLengthStr, 10);

    response.concat("<tr><td>");
    response.concat(description);
    response.concat("</td><td>");
    response.concat(" <TEXTAREA NAME=\"");
    response.concat(token);
    response.concat("\" MAXLENGTH=\"");
    response.concat(maxLengthStr);
    response.concat("\\\">");
    response.concat(value);
    response.concat("</TEXTAREA>");
    response.concat("</td></tr>");
}

void WebCfgServer::buildNavigationButton(String &response, const char *caption, const char *targetPath)
{
    response.concat("<form method=\"get\" action=\"");
    response.concat(targetPath);
    response.concat("\">");
    response.concat("<button type=\"submit\">");
    response.concat(caption);
    response.concat("</button>");
    response.concat("</form>");
}

void WebCfgServer::printParameter(String& response, const char *description, const char *value)
{
    response.concat("<tr>");
    response.concat("<td>");
    response.concat(description);
    response.concat("</td>");
    response.concat("<td>");
    response.concat(value);
    response.concat("</td>");
    response.concat("</tr>");

}

void WebCfgServer::waitAndProcess(const bool blocking, const uint32_t duration)
{
    unsigned long timeout = millis() + duration;
    while(millis() < timeout)
    {
        _server.handleClient();
        if(blocking)
        {
            delay(10);
        }
        else
        {
            delay(50);
        }
    }
}

void WebCfgServer::handleOtaUpload()
{
    if (_server.uri() != "/uploadota") {
        return;
    }
    if(millis() < 60000)
    {
        return;
    }

    esp_task_wdt_init(30, false);

    HTTPUpload& upload = _server.upload();

    if(upload.filename == "")
    {
        Serial.println("Invalid file for OTA upload");
        return;
    }

    if (upload.status == UPLOAD_FILE_START) {
        String filename = upload.filename;
        if (!filename.startsWith("/")) {
            filename = "/" + filename;
        }
        Serial.print("handleFileUpload Name: "); Serial.println(filename);
    } else if (upload.status == UPLOAD_FILE_WRITE) {
        _transferredSize = _transferredSize + upload.currentSize;
        Serial.println(_transferredSize);
        _ota.updateFirmware(upload.buf, upload.currentSize);
    } else if (upload.status == UPLOAD_FILE_END) {
        Serial.println();
        Serial.print("handleFileUpload Size: "); Serial.println(upload.totalSize);
    }
}

void WebCfgServer::sendNewCss()
{
    // escaped by https://www.cescaper.com/
    _server.send(200, "text/plain", newcss);
}

void WebCfgServer::sendFontsInterMinCss()
{
    // escaped by https://www.cescaper.com/
    _server.send(200, "text/plain", intercss);
}
