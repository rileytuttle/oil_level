#include <oil-level-monitor.hpp>
#include <ESP8266WiFi.h>
#include <credentials.h>

#include <Arduino.h>

// #define DEBUG
#ifdef DEBUG
static constexpr uint32_t TANK_LOW_PUBLISH_INTERVAL = 1000 * 10;
static constexpr uint32_t TANK_FILLED_PUBLISH_INTERVAL = 1000 * 10;
static constexpr uint32_t TANK_LEVEL_PUBLISH_INTERVAL = 1000 * 60;
#else
static constexpr uint32_t TANK_LOW_PUBLISH_INTERVAL = 1000 * 60 * 60 * 24; // minimum interval between webhooks publishes
static constexpr uint32_t TANK_FILLED_PUBLISH_INTERVAL = 1000 * 60 * 60 * 24; // minimum interval between webhooks publishes
static constexpr uint32_t TANK_LEVEL_PUBLISH_INTERVAL = 1000 * 60 * 60 * 24 * 3; // only publish the level every few days
#endif

WiFiClientSecure wifiClient;
static uint32_t s_tank_low_event_ts = 0;
static uint32_t s_tank_filled_event_ts = 0;
static uint32_t s_tank_level_update_ts = 0;

namespace IFTTT
{
    const String host = "maker.ifttt.com";
    const int port = 443;
    const char fingerprint[] PROGMEM = "61 62 75 FA EA 5F 64 95 4A F6 09 0F 59 C9 0D E7 1E 6D 66 A3";
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

static void send_ifttt_event(String trigger, String value1, String value2)
{
    connect_wifi();
    wifiClient.setFingerprint(IFTTT::fingerprint);
    wifiClient.setTimeout(15000);
    delay(1000);
    if (!wifiClient.connect(IFTTT::host, IFTTT::port))
    {
        Serial.println("connection failed");
        return;
    }
    Serial.print("wifi client connected to");
    Serial.println(IFTTT::host);

    String Link = "/trigger/"+trigger+
        "/with/key/"+String(Creds::Webhooks::apikey)+
        "?value1="+value1+"&value2="+value2;
    Serial.print("requesting URL: ");
    Serial.println(IFTTT::host+Link);

#ifndef DEBUG
    wifiClient.print(String("GET ") + Link + " HTTP/1.1\r\n" +
        "Host: " + IFTTT::host + "\r\n" +
        "Connection: close\r\n\r\n");
#else
    Serial.println("send compiled out");
#endif

    Serial.println("request sent");

#ifndef DEBUG
    while (wifiClient.connected()) {
        String line = wifiClient.readStringUntil('\n');
        if (line == "\r") {
            Serial.println("headers received");
            break;
        }
    }
    Serial.println("reply was:");
    Serial.println("==========");
    String line;
    while(wifiClient.available()){
        line = wifiClient.readStringUntil('\n');  //Read Line by Line
        Serial.println(line); //Print response
    }
#else
    Serial.println("not going to receive anything");
#endif

    Serial.println("==========");
    Serial.println("closing connection");
    disconnect_wifi();
}

static void webhooks_tank_low_event(OilLevelMonitor::Status status, float level)
{
    const unsigned long current_millis = millis(); 
    if (current_millis - s_tank_low_event_ts > TANK_LOW_PUBLISH_INTERVAL)
    {
        send_ifttt_event("tank_status_change", "tank low", String(level * 100));
        s_tank_low_event_ts = current_millis;
    }
}

static void webhooks_tank_filled_event(OilLevelMonitor::Status status, float level)
{
    const unsigned long current_millis = millis(); 
    if (current_millis - s_tank_filled_event_ts > TANK_FILLED_PUBLISH_INTERVAL)
    {
        send_ifttt_event("tank_status_change", "tank filled", String(level * 100));
        s_tank_filled_event_ts = current_millis;
    }
}

static void publish_tank_level(OilLevelMonitor::Status status, float level)
{
    const unsigned long current_millis = millis();
    if (current_millis - s_tank_level_update_ts > TANK_FILLED_PUBLISH_INTERVAL)
    {
        send_ifttt_event("tank_status_change", OilLevelMonitor::status_to_string(status), String(level * 100));
        s_tank_level_update_ts = current_millis;
    }
}

const OilLevelMonitor::Params params =
{
    .tank_low_cb = webhooks_tank_low_event,
    .tank_filled_cb = webhooks_tank_filled_event,
};

OilLevelMonitor oil_level_monitor(params);

void setup() {
    Serial.begin(115200);
    while (!Serial) delay(10);

    Serial.println(F("Adafruit VL53L1X sensor demo"));

    oil_level_monitor.init();
}

void loop() {
    // const unsigned long current_millis = millis();
    const OilLevelMonitor::Status status = oil_level_monitor.update();
    publish_tank_level(status, oil_level_monitor.get_level());
}
