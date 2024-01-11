#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>

#define HOST "http://192.168.1.105:80"
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

HTTPClient http;
StaticJsonDocument<1024> JSONDoc;

struct Ticket {
    uint GlobalID;
    uint SourceID;
    uint DestinationID;
    uint TicketID;
    uint ResponseID = UINT_MAX;
    const char* Data;
    const char* Date;
};
struct TicketResult {
    bool ok;
    short status;
    Ticket* tickets;
    uint8 t_count;

    TicketResult() {}

    TicketResult(short ticketsCount) {
        tickets = new Ticket[ticketsCount];
        t_count = ticketsCount;
    }
};
struct SecureResult {
    bool ok;
    short status;
    int ID;
};
struct Credentials {
  String username;
  String password;
};
struct TicketServiceResult {
  bool ok;
  short status;
  long count;
};
struct TicketLastResult {
  bool ok;
  short status;
  long offset;
};
struct ApiVersionResult {
  bool ok;
  short status;
  int version;
};

String MakeGETRequest(String URL) {
  WiFiClient client;
  HTTPClient http;
  http.begin(client, URL);

  int status = http.GET();
  if (status != 200) {
    return "null";
  }
 String payload = http.getString();
 http.end();
 return payload;
}

SecureResult Auth(Credentials credits) {
  String URL = String(HOST) + "/api/v2/auth?username=" + credits.username + "&password=" + credits.password;

  String payload = MakeGETRequest(URL);
  if (payload.equals("null")) return SecureResult { .ok = false, .status = -1};

  deserializeJson(JSONDoc, payload);
  return SecureResult { .ok = JSONDoc["ok"], .status = JSONDoc["status"], .ID = JSONDoc["ID"] };
}
SecureResult UpdatePass(Credentials credits, String newpassword) {
  String URL = String(HOST) + "/api/v2/update?username=" + credits.username + "&password=" + credits.password + "&newpassword=" + newpassword;

  String payload = MakeGETRequest(URL);
  if (payload.equals("null")) return SecureResult { .ok = false, .status = -1};

  deserializeJson(JSONDoc, payload);
  return SecureResult { .ok = JSONDoc["ok"], .status = JSONDoc["status"], .ID = JSONDoc["ID"] };
}
SecureResult Create(Credentials credits) {
  String URL = String(HOST) + "/api/v2/create?username=" + credits.username + "&password=" + credits.password;

  String payload = MakeGETRequest(URL);
  if (payload.equals("null")) return SecureResult { .ok = false, .status = -1};

  deserializeJson(JSONDoc, payload);
  return SecureResult { .ok = JSONDoc["ok"], .status = JSONDoc["status"], .ID = JSONDoc["ID"] };
}
SecureResult Delete(Credentials credits) {
  String URL = String(HOST) + "/api/v2/delete?username=" + credits.username + "&password=" + credits.password;

  String payload = MakeGETRequest(URL);
  if (payload.equals("null")) return SecureResult { .ok = false, .status = -1};

  deserializeJson(JSONDoc, payload);
  return SecureResult { .ok = JSONDoc["ok"], .status = JSONDoc["status"], .ID = 0 };
}

TicketResult TicketPush(Credentials credits, Ticket ticket) {
  String URL = String(HOST) + "/api/v1/push?username=" + credits.username + "&password=" + credits.password
  + "&destination=" + ticket.DestinationID
  + "&data=" + ticket.Data;
  if (ticket.ResponseID != UINT_MAX) URL += String("&responseid=") + ticket.ResponseID;

  String payload = MakeGETRequest(URL);
  if (payload.equals("null")) {
    TicketResult tres = TicketResult(0);
    tres.ok = false;
    tres.status = -1;
    return tres;
  }

  deserializeJson(JSONDoc, payload);
  
  if (JSONDoc["ok"]) {
    short size = JSONDoc["tickets"].size();
    TicketResult tres = TicketResult(size);
    tres.ok = true;
    tres.status = JSONDoc["status"];

    for (int i = 0; i < size; i++) {
      tres.tickets[i].GlobalID = JSONDoc["tickets"][0]["GlobalID"];
      tres.tickets[i].SourceID = JSONDoc["tickets"][0]["SourceID"];
      tres.tickets[i].DestinationID = JSONDoc["tickets"][0]["DestinationID"];
      tres.tickets[i].TicketID = JSONDoc["tickets"][0]["TicketID"];
      tres.tickets[i].ResponseID = JSONDoc["tickets"][0]["ResponseID"];
      tres.tickets[i].Data = JSONDoc["tickets"][0]["Data"];
      tres.tickets[i].Date = JSONDoc["tickets"][0]["Date"];
    }

    return tres;
  } else {
    TicketResult tres = TicketResult(0);
    tres.ok = false;
    tres.status = JSONDoc["status"];
    return tres;
  }
}
TicketResult TicketPull(Credentials credits, uint offset = UINT_MAX, short count = USHRT_MAX) {
  String URL = String(HOST) + "/api/v1/pull?username=" + credits.username + "&password=" + credits.password;
  if (offset != UINT_MAX) URL += String("&offset=") + offset;
  if (count != USHRT_MAX) URL += String("&count=") + count;

  String payload = MakeGETRequest(URL);
  if (payload.equals("null")) {
    TicketResult tres = TicketResult(0);
    tres.ok = false;
    tres.status = -1;
    return tres;
  }

  deserializeJson(JSONDoc, payload);

  if (JSONDoc["ok"]) {
    short size = JSONDoc["tickets"].size();
    TicketResult tres = TicketResult(size);
    tres.ok = true;
    tres.status = JSONDoc["status"];

    for (int i = 0; i < size; i++) {
      tres.tickets[i].GlobalID = JSONDoc["tickets"][0]["GlobalID"];
      tres.tickets[i].SourceID = JSONDoc["tickets"][0]["SourceID"];
      tres.tickets[i].DestinationID = JSONDoc["tickets"][0]["DestinationID"];
      tres.tickets[i].TicketID = JSONDoc["tickets"][0]["TicketID"];
      tres.tickets[i].ResponseID = JSONDoc["tickets"][0]["ResponseID"];
      tres.tickets[i].Data = JSONDoc["tickets"][0]["Data"];
      tres.tickets[i].Date = JSONDoc["tickets"][0]["Date"];
    }

    return tres;
  } else {
    TicketResult tres = TicketResult(0);
    tres.ok = false;
    tres.status = JSONDoc["status"];
    return tres;
  }
}
TicketLastResult TicketGetLast(Credentials credits) {
  String URL = String(HOST) + "/api/v1/last?username=" + credits.username + "&password=" + credits.password;

  String payload = MakeGETRequest(URL);
  if (payload.equals("null")) return TicketLastResult {.ok = false, .status = -1, .offset = -1};

  deserializeJson(JSONDoc, payload);
  
  if (JSONDoc["ok"]) return TicketLastResult {.ok = true, .status = JSONDoc["status"], .offset = JSONDoc["offset"]};
  else return TicketLastResult {.ok = false, .status = JSONDoc["status"], .offset = -1};
}
TicketServiceResult TicketFlush(Credentials credits) {
  String URL = String(HOST) + "/api/v1/flush?username=" + credits.username + "&password=" + credits.password;

  String payload = MakeGETRequest(URL);
  if (payload.equals("null")) return TicketServiceResult {.ok = false, .status = -1, .count = -1};

  deserializeJson(JSONDoc, payload);
  
  if (JSONDoc["ok"]) return TicketServiceResult {.ok = true, .status = JSONDoc["status"], .count = JSONDoc["count"]};
  else return TicketServiceResult {.ok = false, .status = JSONDoc["status"], .count = -1};
}

ApiVersionResult GetApiV1Version() {
  String URL = String(HOST) + "/api/v1";

  String payload = MakeGETRequest(URL);
  if (payload.equals("null")) return ApiVersionResult {.ok = false, .status = -1, .version = -1};

  deserializeJson(JSONDoc, payload);
  
  if (JSONDoc["ok"]) return ApiVersionResult {.ok = true, .status = JSONDoc["status"], .version = JSONDoc["version"]};
  else return ApiVersionResult {.ok = false, .status = JSONDoc["status"], .version = -1};
}
ApiVersionResult GetApiV2Version() {
  String URL = String(HOST) + "/api/v2";

  String payload = MakeGETRequest(URL);
  if (payload.equals("null")) return ApiVersionResult {.ok = false, .status = -1, .version = -1};

  deserializeJson(JSONDoc, payload);
  
  if (JSONDoc["ok"]) return ApiVersionResult {.ok = true, .status = JSONDoc["status"], .version = JSONDoc["version"]};
  else return ApiVersionResult {.ok = false, .status = JSONDoc["status"], .version = -1};
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