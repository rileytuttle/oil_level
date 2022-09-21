import paho.mqtt.client as mqtt
import credentials as creds
import requests

MQTT_ADDRESS = "localhost"
MQTT_TOPIC = "home/tank/+"

def on_connect(client, userdata, flags, rc):
    """ The callback for when the client receives a CONNACK response from the server."""
    print('Connected with result code ' + str(rc))
    client.subscribe(MQTT_TOPIC)

def on_message(client, userdata, msg):
    """The callback for when a PUBLISH message is received from the server."""
    print(msg.topic + ' ' + str(msg.payload))
    if msg.topic == "home/tank/status":
        if "Low" in str(msg.payload):
            print("tank low received");
            requests.post(f'https://maker.ifttt.com/trigger/tank_low/with/key/{creds.webhooks_apikey}')

def main():
    mqtt_client = mqtt.Client()
    mqtt_client.username_pw_set(creds.mqtt_username, creds.mqtt_password)
    mqtt_client.on_connect = on_connect
    mqtt_client.on_message = on_message
    mqtt_client.connect(MQTT_ADDRESS, 1883)
    mqtt_client.loop_forever()

if __name__ == '__main__':
    print('MQTT to IFTTT')
    main()
