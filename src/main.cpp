#include "Arduino.h"
#include <tcp/tcp-proto.h>

#define FIRMWARE_BUILD 0
#define EEPROM_SIZE 512

#define SERVICE_USERNAME 		"ledstrip_0afb8683"
#define SERVICE_PASSWORD 		"bSPS81ePSADMMGVhQhMg"
#define WIFI_SSID 					"Barsik_2.4G"
#define WIFI_PASS 					"Home4444"

const char* A_SERVICE_USERNAME = SERVICE_USERNAME;
const char* A_SERVICE_PASSWORD = SERVICE_PASSWORD;

void OnTicket(TicketResult tres);

void setup() {
  Serial.begin(9600);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  if (!Init()) return;

  Credentials creds = Credentials(SERVICE_USERNAME, SERVICE_PASSWORD);
  SecureResult res;
  if (!Auth(creds, &res)) return;

  SetTicketCallback(&OnTicket);
}

void OnTicket(TicketResult tres) {
  for (int i = 0; i < tres.t_count; i++) Serial.println(tres.tickets[i].Data);
}

unsigned int PING_lastMS = 0;
const unsigned int PING_timeoutMS = 7500;
void TimeoutKPLping() {
  if (!IsConnected()) return;
  if (millis() - PING_lastMS > PING_timeoutMS) {
    PING_lastMS = millis();
    Ping();
  }
}

void loop() {
  delay(200);
  Tick();
  TimeoutKPLping();
}