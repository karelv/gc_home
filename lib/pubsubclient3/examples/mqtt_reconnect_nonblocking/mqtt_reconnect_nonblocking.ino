/*
 Reconnecting MQTT example - non-blocking

 This sketch demonstrates how to keep the client connected
 using a non-blocking reconnect function. If the client loses
 its connection, it attempts to reconnect every 5 seconds
 without blocking the main loop.
*/

#include <Ethernet.h>
#include <PubSubClient.h>
#include <SPI.h>

// Update these with values suitable for your hardware/network.
byte mac[] = {0xDE, 0xED, 0xBA, 0xFE, 0xFE, 0xED};
IPAddress ip(172, 16, 0, 100);
IPAddress server(172, 16, 0, 2);

void callback(char* topic, uint8_t* payload, size_t plength) {
    // handle message arrived
}

EthernetClient ethClient;
PubSubClient client(ethClient);

long lastReconnectAttempt = 0;

boolean reconnect() {
    if (client.connect("arduinoClient")) {
        // Once connected, publish an announcement...
        client.publish("outTopic", "hello world");
        // ... and resubscribe
        client.subscribe("inTopic");
    }
    return client.connected();
}

void setup() {
    client.setServer(server, 1883);
    client.setCallback(callback);

    Ethernet.begin(mac, ip);
    delay(1500);
    lastReconnectAttempt = 0;
}

void loop() {
    if (!client.connected()) {
        unsigned long now = millis();
        if (now - lastReconnectAttempt > 5000) {
            lastReconnectAttempt = now;
            // Attempt to reconnect
            if (reconnect()) {
                lastReconnectAttempt = 0;
            }
        }
    } else {
        // Client connected

        client.loop();
    }
}
