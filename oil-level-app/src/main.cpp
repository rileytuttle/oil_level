#include <oil-level-monitor.hpp>
#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <credentials.h>
#include <ESP8266HTTPClient.h>

#include <Arduino.h>
#include <vector>

static constexpr unsigned long TANK_LOW_PUBLISH_INTERVAL = 1000 * 60; // minimum interval between webhooks publishes
static constexpr unsigned long TANK_FILLED_PUBLISH_INTERVAL = 1000 * 60; // minimum interval between webhooks publishes

// WiFiClientSecure wifiClient;
WiFiClient wifiClient;
static unsigned long s_tank_low_event_ts = 0;
static unsigned long s_tank_filled_event_ts = 0;


namespace IFTTT
{
    const String host = "maker.ifttt.com";
    const int port = 443;
}

static void connect_wifi()
{
    Serial.print("Connecting to ");
    Serial.println(Creds::Wifi::ssid);

    // Connect to the WiFi
    {
        // scoping because WiFi being doesn't take in a const char*
        WiFi.begin(Creds::Wifi::ssid, Creds::Wifi::password);
    }
    // Wait until the connection has been confirmed before continuing
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    // Debugging - Output the IP Address of the ESP8266
    Serial.println("WiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
}

static void disconnect_wifi()
{
    WiFi.mode(WIFI_OFF);
}

static void webhooks_tank_low_event(OilLevelMonitor::Status status, float pct)
{
    const unsigned long current_millis = millis(); 
    if (current_millis - s_tank_low_event_ts > TANK_LOW_PUBLISH_INTERVAL)
    {
        connect_wifi();
        // wifiClient.setInsecure();
        if (!wifiClient.connect(IFTTT::host, IFTTT::port))
        {
            Serial.println("connection failed");
            return;
        }
        String jsonString = "";
        jsonString += "{\value1\":\"";
        jsonString += OilLevelMonitor::status_to_string(status);
        jsonString += "\",\"value2\":\"";
        jsonString += String(pct);
        jsonString += "\"}";
        int jsonLength = jsonString.length();
        String lenString = String(jsonLength);
        String postString = "";
        postString += "POST /trigger/";
        postString += "tank_status_change";
        postString += "/with/key/";
        postString += String(Creds::Webhooks::apikey);
        postString += " HTTP/1.1\r\n";
        postString += "Host: ";
        postString += IFTTT::host;
        postString += "\r\n";
        postString += "Content-Type: application/json\r\n";
        postString += "Content-Length: ";
        postString += lenString + "\r\n";
        postString += "\r\n";
        postString += jsonString;
        wifiClient.print(postString);
        wifiClient.stop();
        disconnect_wifi();
    }
}

static void webhooks_tank_filled_event(OilLevelMonitor::Status status, float pct)
{
    const unsigned long current_millis = millis(); 
    if (current_millis - s_tank_filled_event_ts > TANK_FILLED_PUBLISH_INTERVAL)
    {
        connect_wifi();
        // wifiClient.setInsecure();
        // if (!wifiClient.connect(IFTTT::host, IFTTT::port))
        if (!wifiClient.connect(IFTTT::host, 80))
        {
            Serial.println("connection failed");
            return;
        }
        String get_string = "";
        get_string += "GET ";
        get_string += "/trigger/tank_status_change/with/key/";
        get_string += String(Creds::Webhooks::apikey);
        get_string += "?Value1=Filled&Value2=";
        get_string += String(pct);
        get_string += " HTTP/1.1\r\n";
        get_string += "Host: ";
        get_string += IFTTT::host;
        get_string += "\r\n";
        get_string += "Connection: close\r\n";
        wifiClient.print(get_string);
        wifiClient.stop();
        disconnect_wifi();
    }
}

OilLevelMonitor oil_level_monitor(webhooks_tank_low_event, webhooks_tank_filled_event);
// OilLevelMonitor oil_level_monitor; // use this for testing

void setup() {
    Serial.begin(115200);
    while (!Serial) delay(10);

    Serial.println(F("Adafruit VL53L1X sensor demo"));

    oil_level_monitor.init();
}

void loop() {
    // const unsigned long current_millis = millis();
    const OilLevelMonitor::Status prev_status = oil_level_monitor.get_status();
    (void) prev_status;
    const OilLevelMonitor::Status status = oil_level_monitor.update();
    (void) status;
}
