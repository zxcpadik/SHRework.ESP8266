#ifndef __TCPWRAP__
#define __TCPWRAP__

#include "tcp-proto.h"

WiFiClient tcp_client;
bool sync(bool _write);

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
  tcp_client.setTimeout(15000);
  tcp_client.setNoDelay(true);

  if (tcp_client.connect(HOST_MAIN, PORT)) goto con;
  else if (tcp_client.connect(HOST_BACKUP, PORT)) goto con;
  else return false;
  con:

  int bytesg = tcp_client.write(PROTO_GUARD);
  if (bytesg != strlen(PROTO_GUARD)) return false;

  byte* bytes = hexStringToBytes(APP_HASH, APP_HASH_SIZE);
  int bytesh = tcp_client.write(bytes, APP_HASH_SIZE);
  delete bytes;
  if (bytesh != APP_HASH_SIZE) return false;

  if (hash) free(hash);
  hash = malloc(SESSION_HASH_SIZE + 1);
  memset(hash, 0x0, SESSION_HASH_SIZE + 1);
  int bytesc = tcp_client.readBytes((uint8_t*)hash, SESSION_HASH_SIZE);
  if (bytesc != SESSION_HASH_SIZE) return false;

  return IsConnected();
}
bool IsConnected() {
  bool res = tcp_client.connected();
  if (!res) sig_TCP_down();
  return res;
}
const char* GetHash() {
  return (const char*)hash;
}
bool ReadPacket(RawDataPacket& p) {
  if (!IsConnected()) return false;

  uint32_t data_size = 0;
  uint32_t bytes_s = (uint32_t)tcp_client.readBytes((uint8_t*)&data_size, 4);
  if (bytes_s != 4) return false;
  p._alloc(data_size);
  if (data_size == 0) return true;

  uint32_t bytes_d = (uint32_t)tcp_client.readBytes((uint8_t*)p.buf, data_size);
  if (bytes_d == data_size) return true;

  return false;
}
bool WritePacket(RawDataPacket& p) {
  if (!IsConnected()) return false;
  
  uint32_t data_size = p.buf_size;
  uint32_t bytes_s = tcp_client.write((uint8_t*)&data_size, 4);
  if (bytes_s != 4) return false;

  if (p.buf_size == 0) return true;
  uint32_t bytes_d = tcp_client.write((uint8_t*)p.buf, data_size);
  return bytes_d == data_size;
}

bool ReadPacketEx(RawDataPacket& p) {
  if (!sync(false)) return false;
  return ReadPacket(p);
}
bool WritePacketEx(RawDataPacket& p) {
  if (!sync(true)) return false;
  return WritePacket(p);
}

size_t cWrite(unsigned int val) {
  return tcp_client.write((uint8_t*)&val, 4);
}

void (*on_ticket)(TicketResult& t);
void SetTicketCallback(void (*callback)(TicketResult& t)) {
  on_ticket = callback;
}

bool Incoming_Auth(RawDataPacket& p) { return false; }
bool Incoming_AuthData(RawDataPacket& p) { return false; }
bool Incoming_Ticket(RawDataPacket& p) {
  TicketResult tres;
  if (!tres.from_packet(p)) return false;

  if (on_ticket) on_ticket(tres);
  tres._free();

  return true;
}
bool Incoming_TicketService(RawDataPacket& p) { return true; }

void sig_TCP_down() {
  errored = true;
}
bool ProcessPacket(RawDataPacket& p) {
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
  if (tcp_client.available() > 0) {
    Serial.print("Tick() MEM: ");
    Serial.println(ESP.getFreeHeap());
    
    RawDataPacket p;
    if (!ReadPacketEx(p)) {
      sig_TCP_down();
      return false;
    }

    bool res = ProcessPacket(p);

    RawDataPacket px = RawDataPacket(0);
    WritePacket(px);

    if (!res) sig_TCP_down();
    return res;
  }
  return false;
}
bool Ping() {
  if (!IsConnected()) return false;

  ushort _cid = Ping_CID;
  RawDataPacket p = RawDataPacket(Ping_SIZE);
  memcpy(p.buf, (uint8_t*)&_cid, 2);
  if (!WritePacketEx(p)) return false;

  if (!ReadPacket(p)) {
    return false;
  }
  if (p.buf_size != 2) {
    return false;
  }
  
  bool ok = memcmp((void*)&_cid, p.buf, 2) == 0;
  return ok;
}

bool sync(bool _write) {
  if (!IsConnected()) return false;

  if (tcp_client.available() > 0) {
    uint rsig = 0;
    tcp_client.readBytes((uint8_t*)&rsig, 4);
    
    switch (rsig) {
      case SIG_MASTER_DATA_READ: cWrite(_write ? SIG_SLAVE_DATA_WRITE : SIG_SLAVE_DATA_READ); return _write;
      case SIG_MASTER_DATA_WRITE: cWrite(_write ? SIG_SLAVE_DATA_WRITE : SIG_SLAVE_DATA_READ); return true;
      case SIG_MASTER_VOID_READ: cWrite(_write ? SIG_SLAVE_DATA_WRITE : SIG_SLAVE_DATA_READ); return _write;
      case SIG_MASTER_VOID_WRITE: cWrite(_write ? SIG_SLAVE_DATA_WRITE : SIG_SLAVE_DATA_READ); return true;
    
      default: return false;
    }
  } else {
    cWrite(_write ? SIG_SLAVE_VOID_WRITE : SIG_SLAVE_VOID_READ);

    uint rsig = 0;
    tcp_client.readBytes((uint8_t*)&rsig, 4);

    switch (rsig) {
      case SIG_MASTER_DATA_READ: return _write;
      case SIG_MASTER_DATA_WRITE: return true;
      case SIG_MASTER_VOID_READ: return _write;
      case SIG_MASTER_VOID_WRITE: return true;
    
      default: return false;
    }
  }
}

bool Auth(Credentials& credits, SecureResult& res) {
  RawDataPacket p;
  credits.to_packet(p);
  if (!WritePacketEx(p)) return false;

  if (!ReadPacket(p)) return false;
  res.from_packet(p);
  
  return true;
}

WiFiClient getClient() {
  return tcp_client;
}

#endif