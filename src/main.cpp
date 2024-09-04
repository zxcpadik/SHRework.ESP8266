#include "Arduino.h"
#include <shr/tcp-proto.h>
#include <Adafruit_SSD1306.h>

#define FIRMWARE_BUILD 0
#define EEPROM_SIZE 512

#define SERVICE_USERNAME 		"ledstrip_0afb8683"
#define SERVICE_PASSWORD 		"bSPS81ePSADMMGVhQhMg"
#define WIFI_SSID 					"Barsik_2.4G"
#define WIFI_PASS 					"Home4444"

const char* A_SERVICE_USERNAME = SERVICE_USERNAME;
const char* A_SERVICE_PASSWORD = SERVICE_PASSWORD;

Adafruit_SSD1306 display = Adafruit_SSD1306(128, 64, &Wire);

void OnTicket(TicketResult& tres);

void D_println(String str) {
  display.println(str);
  display.display();
}
void D_print(String str) {
  display.print(str);
  display.display();
}
void D_println(const char* str) {
  display.println(str);
  display.display();
}
void D_print(const char* str) {
  display.print(str);
  display.display();
}
void D_Clear() {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.display();
}

void setup() {
  Serial.begin(9600);


  Wire.begin(5, 4);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("Failed to init display");
    while (1) {}
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setFont();
  display.setCursor(0, 0);
  display.display();
  D_println("[INIT] Started");

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  D_print("[WiFi] Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    D_print(".");
  }
  D_println("");
  D_println("[WiFi] Connected!");

  if (!Init()) {
    D_println("[SHR] Init ERROR");
    delay(5000);
    return ESP.restart();
  }
  D_println("[SHR] Init OK");

  Credentials creds = Credentials(SERVICE_USERNAME, SERVICE_PASSWORD);
  SecureResult res;
  if (!Auth(creds, res)) {
    display.print("[SHR] Auth error: ");
    display.println(res.status);
    display.display();
    delay(5000);
    return ESP.restart();
  }
  display.print("[SHR] Auth OK: ");
  display.println(res.userid);
  display.display();

  SetTicketCallback(&OnTicket);
  delay(1000);
  D_Clear();
}

void OnTicket(TicketResult& tres) {
  for (int i = 0; i < tres.t_count; i++) {
    if (strcmp(tres.tickets[i].Data, "clear") == 0) {
      D_Clear();
      continue;
    }

    Serial.println(tres.tickets[i].Data);
    display.println(tres.tickets[i].Data);
  }
  display.display();
}

unsigned int PING_lastMS = 0;
const unsigned int PING_timeoutMS = 7000;
void TimeoutKPLping() {
  if (!IsConnected()) return;
  if (millis() - PING_lastMS > PING_timeoutMS) {
    PING_lastMS = millis();
    Ping();
  }
}

unsigned int TICK_lastMS = 0;
const unsigned int TICK_timeoutMS = 100;
void TimeoutTick() {
  if (!IsConnected()) return;
  if (millis() - TICK_lastMS > TICK_timeoutMS) {
    TICK_lastMS = millis();
    if (Tick()) PING_lastMS = millis();
  }
}

void loop() {
  TimeoutTick();
  TimeoutKPLping();
}