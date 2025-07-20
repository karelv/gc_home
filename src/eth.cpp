#include <Arduino.h>
#include <QNEthernet.h>             // ethernet/MQTT/webserver
#include <qnethernet/QNDNSClient.h>             // ethernet/MQTT/webserver

#include "eth.h"
#include "ntp_lib.h"
#include "ntp_app.h"
#include "mqtt.h"
#include "teensy_rtc15.h"
#include "pins.h"

using namespace qindesign::network; // ethernet/MQTT/webserver

const IPAddress DNS_SERVER{192, 168, 0, 1};
const IPAddress DNS_SERVER2{8, 8, 8, 8};


bool
init_ethernet() 
{
  Serial.printf("Init ethernet...\n");


  pinMode(PIN_LED_ETH, OUTPUT);
  digitalWrite(PIN_LED_ETH, LOW);

  // Unlike the Arduino API (which you can still use), QNEthernet uses
  // the Teensy's internal MAC address by default, so we can retrieve
  // it here
  uint8_t mac[6];
  Ethernet.macAddress(mac);  // This is informative; it retrieves, not sets
  Serial.printf("MAC = %02x:%02x:%02x:%02x:%02x:%02x\r\n",
         mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  DNSClient::setServer(0, DNS_SERVER);
  DNSClient::setServer(1, DNS_SERVER2);

  // Add listeners
  // It's important to add these before doing anything with Ethernet
  // so no events are missed.

  // Listen for link changes
  Ethernet.onLinkState([](bool state) {
    Serial.printf("[Ethernet] Link %s\r\n", state ? "ON" : "OFF");
    if (state)
    {
      digitalWrite(PIN_LED_ETH, HIGH);
      // every re-connect re-synch the time with NTP server
      // todo: NTP
      mqtt_reconnect_handler(NULL);
      // ntp_begin();
      // ntp_app_begin();
      // ntp_app_trigger_synch();
    } else
    {
      digitalWrite(PIN_LED_ETH, LOW);
      Serial.printf("[Ethernet] Link OFF, inform MQTT\n");
      mqtt_disconnect();
    }

    // DNSClient::setServer(0, DNS_SERVER);
  });

  // Listen for address changes
  Ethernet.onAddressChanged([]() {
    IPAddress ip = Ethernet.localIP();
    bool hasIP = (ip != INADDR_NONE);
    if (hasIP) {
      Serial.printf("[Ethernet] Address changed:\r\n");

      Serial.printf("    Local IP = %u.%u.%u.%u\r\n", ip[0], ip[1], ip[2], ip[3]);
      ip = Ethernet.subnetMask();
      Serial.printf("    Subnet   = %u.%u.%u.%u\r\n", ip[0], ip[1], ip[2], ip[3]);
      ip = Ethernet.gatewayIP();
      Serial.printf("    Gateway  = %u.%u.%u.%u\r\n", ip[0], ip[1], ip[2], ip[3]);
      ip = Ethernet.dnsServerIP();
      if (ip != INADDR_NONE) {  // May happen with static IP
        Serial.printf("    DNS      = %u.%u.%u.%u\r\n", ip[0], ip[1], ip[2], ip[3]);
      }
    } else {
      Serial.printf("[Ethernet] Address changed: No IP address\r\n");
    }
  });

  // Static IP
  unsigned long start_time = micros();
  Ethernet.setHostname(TEENSY_NAME);

  MDNS.begin(TEENSY_NAME); //.local Domain name and number of services
  //  MDNS.addService("_https._tcp", 443); //Uncomment for TLS
  // MDNS.addService("_http._tcp", 80);
  MDNS.addService("url", "_http._tcp", 80);


  Serial.printf("Starting Ethernet with static IP...\r\n");
  if (!Ethernet.begin(TEENSY_STATIC_IP, SUBNETMASK, GATEWAY)) {
    Serial.printf("Failed to start Ethernet\r\n");
    return false;
  }

  // When setting a static IP, the address is changed immediately,
  // but the link may not be up; optionally wait for the link here
  if (ETH_LINK_TIMEOUT > 0) {
    Serial.printf("Waiting for link...\r\n");
    if (!Ethernet.waitForLink(ETH_LINK_TIMEOUT)) {
      Serial.printf("No link yet\r\n");
      // We may still see a link later, after the timeout, so
      // continue instead of returning
    }
  }

  unsigned long stop_time = micros();
  Serial.print("init ethernet: ");
  Serial.print(stop_time - start_time);
  Serial.println(" us");
  start_time = micros();

  return true;
}
