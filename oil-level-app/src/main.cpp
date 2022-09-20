#include <oil-level-monitor.hpp>
// #include <melody-player.hpp>
#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <credentials.h>

#include <Arduino.h>
#include <vector>

namespace Mqtt
{
    const char* server = "192.168.1.235";
    const char* clientID = "tank_monitor_client";
    const char* tank_level_topic = "home/tank/level";
    const char* tank_status_topic = "home/tank/status";
}

WiFiClient wifiClient;
PubSubClient client(Mqtt::server, 1883, wifiClient);

OilLevelMonitor oil_level_monitor;

unsigned long mqtt_prev_publish = 0;


// // notes in the melody:
// // note durations: 4 = quarter note, 8 = eighth note, etc.:
// const std::vector<std::pair<int, float>> MELODY =
// {
//     {NOTE_C4, 0.25f},
//     {NOTE_G3, 0.125f},
//     {NOTE_G3, 0.125f},
//     {NOTE_A3, 0.25f},
//     {NOTE_G3, 0.25f},
//     {0,       0.25f},
//     {NOTE_B3, 0.25f},
//     {NOTE_C4, 0.25f},
// };
static void connect_MQTT()
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
    // Connect to MQTT Broker
    // client.connect returns a boolean value to let us know if the connection was successful.
    // If the connection is failing, make sure you are using the correct MQTT Username and Password (Setup Earlier in the Instructable)
    if (client.connect(Mqtt::clientID, Creds::Mqtt::username, Creds::Mqtt::password))
    {
        Serial.println("Connected to MQTT Broker!");
    }
    else
    {
        Serial.println("Connection to MQTT Broker failed...");
    }
}

static void mqtt_publish(OilLevelMonitor::Status status, float level)
{
    connect_MQTT();
    String tank_level_string = "level: "+String((float)(level * 100))+"%";
    String status_st = OilLevelMonitor::status_to_string(status);
    String tank_status_string = "status: "+status_st;
    if (client.publish(Mqtt::tank_level_topic, tank_level_string.c_str()))
    {
        Serial.println("tank level sent");
    }
    else
    {
        Serial.println("failed to send tank level");
    }

    if (client.publish(Mqtt::tank_status_topic, tank_status_string.c_str()))
    {
        Serial.println("tank status sent");
    }
    else
    {
        Serial.println("failed to send tank status");
    }
    client.disconnect();
}

void setup() {
    Serial.begin(115200);
    while (!Serial) delay(10);

    Serial.println(F("Adafruit VL53L1X sensor demo"));

    oil_level_monitor.init();
}

void loop() {
    const unsigned long current_millis = millis();
    const OilLevelMonitor::Status prev_status = oil_level_monitor.get_status();
    (void)prev_status;

    const OilLevelMonitor::Status status = oil_level_monitor.update();
    (void)status;

    if (current_millis - mqtt_prev_publish > 1000 * 60)
    {
        mqtt_publish(status, oil_level_monitor.get_level());
        mqtt_prev_publish = current_millis;
    }
}
