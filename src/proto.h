#ifndef __PROTO__
#define __PROTO__

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>

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
struct ApiVersionResult {
  bool ok;
  short status;
  int version;
};

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

  TicketResult();
  TicketResult(short ticketsCount);
};


String MakeGETRequest(String URL);

SecureResult Auth(Credentials credits);
SecureResult UpdatePass(Credentials credits, String newpassword);
SecureResult Create(Credentials credits);
SecureResult Delete(Credentials credits);

TicketResult TicketPush(Credentials credits, Ticket ticket);
TicketResult TicketPull(Credentials credits, uint offset = UINT_MAX, short count = SHRT_MAX);
TicketServiceResult TicketGetLast(Credentials credits);
TicketServiceResult TicketFlush(Credentials credits);

ApiVersionResult GetApiV1Version();
ApiVersionResult GetApiV2Version();

#endif