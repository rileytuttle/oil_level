#include <ESP8266WiFi.h>
#include <credentials.h>
#include <Arduino.h>

namespace IFTTT
{
    const String host = "maker.ifttt.com";
    const int port = 443;
}

WiFiClientSecure wifiClient;

const char fingerprint[] PROGMEM = "61 62 75 FA EA 5F 64 95 4A F6 09 0F 59 C9 0D E7 1E 6D 66 A3";

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

static void send_ifttt_event()
{
    connect_wifi();
    wifiClient.setFingerprint(fingerprint);
    wifiClient.setTimeout(15000);
    delay(1000);
    if (!wifiClient.connect(IFTTT::host, IFTTT::port))
    {
        Serial.println("connection failed");
        return;
    }
    Serial.print("wifi client connected to");
    Serial.println(IFTTT::host);

    String Link = "/trigger/tank_status_change/with/key/"+
        String(Creds::Webhooks::apikey)+"?value1=Filled&value2=88";
    Serial.print("requesting URL: ");
    Serial.println(IFTTT::host+Link);

    wifiClient.print(String("GET ") + Link + " HTTP/1.1\r\n" +
        "Host: " + IFTTT::host + "\r\n" +               
        "Connection: close\r\n\r\n");

    Serial.println("request sent");

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
    Serial.println("==========");
    Serial.println("closing connection");
}

void setup()
{
    Serial.begin(115200);
    while (!Serial) delay(10);

    send_ifttt_event();
}

void loop() {}
