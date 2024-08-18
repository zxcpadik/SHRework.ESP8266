#ifndef __TCPWRAP__
#define __TCPWRAP__

#include "tcp-proto.h"

bool sync(bool _write);
WiFiClient client;

byte* hexStringToBytes(const char* hexString, int length) {
  int len = strlen(hexString);
  length = len / 2;
  byte* byteArray = new byte[length];

  for (int i = 0; i < len; i += 2) {
    char hexByte[3];
    hexByte[0] = hexString[i];
    hexByte[1] = hexString[i + 1];
    hexByte[2] = '\0';
    byteArray[i / 2] = (byte)strtoul(hexByte, NULL, 16);
  }

  return byteArray;
}

bool errored = false;
void* hash = nullptr;
bool Init() {
  errored = false;
  client.setTimeout(15000);
  client.setNoDelay(true);

  if (client.connect(HOST_MAIN, PORT)) goto con;
  else if (client.connect(HOST_BACKUP, PORT)) goto con;
  else return false;
  con:

  int bytesg = client.write(PROTO_GUARD);
  if (bytesg != strlen(PROTO_GUARD)) return false;

  byte* bytes = hexStringToBytes(APP_HASH, APP_HASH_SIZE);
  int bytesh = client.write(bytes, APP_HASH_SIZE);
  if (bytesh != APP_HASH_SIZE) return false;
  delete bytes;

  if (hash) free(hash);
  hash = malloc(SESSION_HASH_SIZE + 1);
  memset(hash, 0x0, SESSION_HASH_SIZE + 1);
  int bytesc = client.readBytes((uint8_t*)hash, SESSION_HASH_SIZE);
  if (bytesc != SESSION_HASH_SIZE) return false;

  return IsConnected();
}
bool IsConnected() {
  bool res = client.connected();
  if (!res) sig_TCP_down();
  return res;
}
const char* GetHash() {
  return (const char*)hash;
}
bool ReadPacket(RawDataPacket *p) {
  if (!IsConnected()) return false;

  uint32_t data_size = 0;
  uint32_t bytes_s = (uint32_t)client.readBytes((uint8_t*)&data_size, 4);
  if (bytes_s != 4) return false;

  p->Init(data_size);
  uint32_t bytes_d = (uint32_t)client.readBytes((uint8_t*)p->buf, data_size);
  if (bytes_d == data_size) return true;

  p->_free();
  return false;
}
bool WritePacket(RawDataPacket *p) {
  if (!IsConnected()) return false;
  
  uint32_t data_size = p->buf_size;
  uint32_t bytes_s = client.write((uint8_t*)&data_size, 4);
  if (bytes_s != 4) return false;

  uint32_t bytes_d = client.write((uint8_t*)p->buf, data_size);
  return bytes_d == data_size;
}

bool ReadPacketEx(RawDataPacket *p) {
  if (!sync(false)) return false;
  return ReadPacket(p);
}
bool WritePacketEx(RawDataPacket *p) {
  if (!sync(true)) return false;
  return WritePacket(p);
}

size_t cWrite(unsigned int val) {
  return client.write((uint8_t*)&val, 4);
}

void (*on_ticket)(TicketResult t);
void SetTicketCallback(void (*callback)(TicketResult t)) {
  on_ticket = callback;
}

bool Incoming_Auth(RawDataPacket p) { return false; }
bool Incoming_AuthData(RawDataPacket p) { return false; }
bool Incoming_Ticket(RawDataPacket p) {
  TicketResult tres = TicketResult(0);
  if (!tres.from_packet(p)) return false;

  if (on_ticket) on_ticket(tres);

  tres._free();
  RawDataPacket rp = RawDataPacket(0);
  WritePacket(&rp);
  rp._free();

  return true;
}
bool Incoming_TicketService(RawDataPacket p) { return true; }

void sig_TCP_down() {
  errored = true;
}
bool ProcessPacket(RawDataPacket p) {
  ushort cid = p.getcid();

  bool result = false;
  switch (cid) {
    case SecureResult_CID: result = Incoming_Auth(p); break;
    case TicketResult_CID: result = Incoming_Ticket(p); break;
    case Credentials_CID: result = Incoming_AuthData(p); break;
    case TicketServiceResult_CID: result = Incoming_TicketService(p); break;
    // ++++

    default: break;
  }
  if (!result) sig_TCP_down();
  return result;
}
bool Tick() {
  if (client.available() > 0) {
    RawDataPacket p;
    if (!ReadPacketEx(&p)) { 
      sig_TCP_down();
      return false;
    }
    bool res = ProcessPacket(p);
    p._free();
    if (!res) sig_TCP_down();
    return res;
  }
  return true;
}
bool Ping() {
  if (!IsConnected()) return false;

  ushort _cid = Ping_CID;
  RawDataPacket p = RawDataPacket(Ping_SIZE);
  memcpy(p.buf, (uint8_t*)&_cid, 2);
  if (!WritePacketEx(&p)) return false;
  p._free();

  if (!ReadPacket(&p)) return false;
  if (p.buf_size != 2) return false;
  
  return memcmp((void*)&_cid, p.buf, 2) == 0;
}

bool sync(bool _write) {
  if (!IsConnected()) return false;

  if (client.available() > 0) {
    uint rsig = 0;
    if (client.readBytes((uint8_t*)&rsig, 4) != 4) return false;
    
    if (rsig == SIG_RWRITE) return cWrite(_write ? SIG_CROSS : SIG_READ) == 4;
    return false;
  } else {
    if (cWrite(_write ? SIG_WRITE : SIG_READ) != 4) return false;

    uint rsig = 0;
    if (client.readBytes((uint8_t*)&rsig, 4) != 4) return false;

    if (rsig == SIG_RREAD && _write) return true;
    else return false;
  }
}

bool Auth(Credentials credits, SecureResult* res) {
  RawDataPacket pout = credits.to_packet();
  if (!WritePacketEx(&pout)) return false;
  pout._free();

  RawDataPacket pinp = RawDataPacket();
  if (!ReadPacket(&pinp)) return false;
  res->from_packet(pinp);
  pinp._free();
  
  return true;
}
#endif