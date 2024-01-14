#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>
#include "proto.h"

#define SERVICE_USERNAME "esptest"
#define SERVICE_PASSWORD "testpass"
#define WIFI_SSID "Barsik_2.4G"
#define WIFI_PASS "Home4444"

void Panic() {
  bool status = false;
    for (int i = 0; i < 60; i++) {
      digitalWrite(LED_BUILTIN, status);
      status = !status;
      delay(500);
    }
    ESP.restart();
}


const Credentials credits = Credentials { .username = SERVICE_USERNAME, .password = SERVICE_PASSWORD};
uint lastID = 1;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(250);
    digitalWrite(LED_BUILTIN, LOW);
    delay(250);
  }

  digitalWrite(LED_BUILTIN, HIGH);
  delay(1000);

  TicketLastResult tlastres = TicketGetLast(credits);
  if (!tlastres.ok) Panic();
  lastID = tlastres.offset;
}

unsigned long _pull_Last = 0;
unsigned long _pull_Timeout = 200;

void loop() {
  if (millis() - _pull_Last > _pull_Timeout) {
    _pull_Last = millis();

    TicketResult result = TicketPull(credits, lastID, 1);
    if (result.t_count > 0) {
      lastID += result.t_count;
      //WHATEVER HERE
      
    }
    delete result.tickets;
  }
}