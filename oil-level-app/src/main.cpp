#include <oil-level-monitor.hpp>
#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <credentials.h>
#include <ESP8266HTTPClient.h>

#include <Arduino.h>
#include <vector>

static constexpr unsigned long TANK_LOW_PUBLISH_INTERVAL = 1000 * 60; // minimum interval between webhooks publishes
static constexpr unsigned long TANK_FILLED_PUBLISH_INTERVAL = 1000 * 60; // minimum interval between webhooks publishes

WiFiClient wifiClient;
unsigned long webhooks_prev_publish = 0;

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

static void webhooks_tank_low_event(float pct)
{
    const unsigned long current_millis = millis(); 
    if (current_millis - s_tank_low_event_ts > TANK_LOW_PUBLISH_INTERVAL)
    {
        connect_wifi();
        HTTPClient http;
        http.begin(wifiClient, "https://maker.ifttt.com/trigger/tank_low/with/key"+String(Creds::Webhooks::apikey)+"?value1="+pct);
        http.GET();
        http.end();
        disconnect_wifi();
    }
}

static void webhooks_tank_filled_event(float pct)
{
    const unsigned long current_millis = millis(); 
    if (current_millis - s_tank_filled_event_ts > TANK_FILLED_PUBLISH_INTERVAL)
    {
        connect_wifi();
        HTTPClient http;
        http.begin(wifiClient, "https://maker.ifttt.com/trigger/tank_filled/with/key"+String(Creds::Webhooks::apikey)+"?value1="+pct);
        http.GET();
        http.end();
        disconnect_wifi();
    }
}

// OilLevelMonitor oil_level_monitor(webhooks_tank_low_event, webhooks_tank_filled_event);
OilLevelMonitor oil_level_monitor; // use this for testing

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
