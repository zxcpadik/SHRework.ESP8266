#include <Arduino.h>
#include <climits>
#include <ESP8266WiFi.h>
#include "../system/safemem.h"

#ifndef __TCPPROTO__
#define __TCPPROTO__

#define HOST_MAIN "sh-rework.ru"
#define HOST_BACKUP "192.168.1.105"
#define PORT 8090
#define APP_HASH "58e57fa170955c5f"
#define APP_HASH_SIZE 8
#define SESSION_HASH_SIZE 32
#define PROTO_GUARD "SHRework"

#define SIG_READ    100777100
#define SIG_RREAD   200777200
#define SIG_WRITE   300777300
#define SIG_RWRITE  400777400
#define SIG_CROSS   500777500

#define SecureResult_SIZE 9
#define SecureResult_CID 13

#define Credentials_SIZE 10
#define Credentials_CID 12

#define TicketServiceResult_SIZE 9
#define TicketServiceResult_CID 11

#define TicketResult_SIZE 6
#define TicketResult_CID 10

#define SetValue_SIZE 0
#define GetValue_SIZE 0
#define GetSchema_SIZE 0

#define SetValue_CID 20
#define GetValue_CID 21
#define GetSchema_CID 22

#define Ping_SIZE 2
#define Ping_CID 5

#define Ticket_SIZE 28

struct RawDataPacket {
  int GetSize() {
    return buf_size + 4;
  }
  uint32_t buf_size;
  void* buf;

  ushort getcid() {
    ushort ret = 0;
    uint8_t* lptr = (uint8_t*)(this->buf);
    if (buf_size >= 2) memcpy(&ret, &lptr[0], 2);
    return ret;
  }
  bool getok() {
    bool ret = false;
    uint8_t* lptr = (uint8_t*)(this->buf);
    if (buf_size >= 3) memcpy(&ret, &lptr[2], 1);
    return ret;
  }
  short getstatus() {
    short ret = 0;
    uint8_t* lptr = (uint8_t*)(this->buf);
    if (buf_size >= 5) memcpy(&ret, &lptr[3], 2);
    return ret;
  }

  void _free() {
    safe_free(buf);
    buf_size = 0;
    this->buf = nullptr;
  }
  void Init(int size) {
    buf = safe_alloc(size);
    this->buf_size = size;
  }
  RawDataPacket() {}
  RawDataPacket(int size) { Init(size); }
  ~RawDataPacket() {
    _free();
  }
};

struct SecureResult {
  uint16_t cid = SecureResult_CID;
  bool ok;
  int16_t status;
  int32_t userid;

  RawDataPacket to_packet() {
    RawDataPacket p = RawDataPacket(SecureResult_SIZE);

    uint8_t* lptr = (uint8_t*)p.buf;
    memcpy(&lptr[0], (void*)&cid, 2);
    memcpy(&lptr[2], (void*)&ok, 1);
    memcpy(&lptr[3], (void*)&status, 2);
    memcpy(&lptr[5], (void*)&userid, 4);

    return p;
  }
  bool from_packet(RawDataPacket p) {
    if (p.buf_size != SecureResult_SIZE) return false;

    uint8_t* lptr = (uint8_t*)p.buf;
    ushort _cid = 0;
    memcpy((void*)&_cid, &lptr[0], 2);
    if (_cid != SecureResult_CID) return false;
    cid = _cid;

    memcpy((void*)&ok, &lptr[2], 1);
    memcpy((void*)&status, &lptr[3], 2);
    memcpy((void*)&userid, &lptr[5], 4);

    return true;
  }
};
struct Credentials {
  uint16_t cid = Credentials_CID;
  uint32_t username_len;
  uint32_t password_len;
  char* username;
  char* password;

  String getUsername() {
    return String(username);
  }
  String getPassword() {
    return String(username);
  }

  Credentials() {}
  Credentials(String _username, String _password) {
    this->username = safe_strdup(_username.c_str());
    this->password = safe_strdup(_password.c_str());

    this->username_len = strlen(this->username);
    this->password_len = strlen(this->password);
  }
  Credentials(const char* _username, const char* _password) {
    this->username = safe_strdup(_username);
    this->password = safe_strdup(_password);

    this->username_len = strlen(_username);
    this->password_len = strlen(_password);
  }

  size_t getUsernameLen() { return strlen(this->username); }
  size_t getPasswordLen() { return strlen(this->password); }

  RawDataPacket to_packet() {
    size_t ulen = this->getUsernameLen();
    size_t plen = this->getPasswordLen();

    int size = Credentials_SIZE + ulen + plen;
    RawDataPacket p = RawDataPacket(size);

    uint8_t* lptr = (uint8_t*)p.buf;
    memcpy(&lptr[0], (void*)&cid, 2);
    memcpy(&lptr[2], (void*)&ulen, 4);
    memcpy(&lptr[6], (void*)&plen, 4);
    memcpy(&lptr[10], (void*)username, ulen);
    memcpy(&lptr[10 + ulen], (void*)password, plen);

    return p;
  }
  bool from_packet(RawDataPacket p) {
    if (p.buf_size != Credentials_SIZE) return false;

    uint8_t* lptr = (uint8_t*)p.buf;
    ushort _cid = 0;
    memcpy((void*)&_cid, &lptr[0], 2);
    if (_cid != Credentials_CID) return false;
    cid = _cid;

    memcpy((void*)&username_len, &lptr[2], 4);
    memcpy((void*)&password_len, &lptr[6], 4);

    username_len += (uint32_t)(lptr[9 + username_len] != 0);
    password_len += (uint32_t)(lptr[9 + username_len + password_len] != 0);

    username = (char*)safe_alloc(username_len);
    password = (char*)safe_alloc(password_len);

    memcpy(username, &lptr[10], username_len);
    memcpy(password, &lptr[10 + username_len], password_len);
    return true;
  }

  void _free() {
    safe_free(username);
    safe_free(password);
  }
  ~Credentials() { _free(); }
};
struct TicketServiceResult {
  uint16_t cid = Credentials_CID;
  bool ok;
  int16_t status;
  int32_t count;

  RawDataPacket to_packet() {
    RawDataPacket p = RawDataPacket(SecureResult_SIZE);

    uint8_t* lptr = (uint8_t*)p.buf;
    memcpy(&lptr[0], (void*)&cid, 2);
    memcpy(&lptr[2], (void*)&ok, 1);
    memcpy(&lptr[3], (void*)&status, 2);
    memcpy(&lptr[5], (void*)&count, 4);

    return p;
  }
  bool from_packet(RawDataPacket p) {
    if (p.buf_size != SecureResult_SIZE) return false;

    uint8_t* lptr = (uint8_t*)p.buf;
    ushort _cid = 0;
    memcpy((void*)&_cid, &lptr[0], 2);
    if (_cid != SecureResult_CID) return false;
    cid = _cid;

    memcpy((void*)&ok, &lptr[2], 1);
    memcpy((void*)&status, &lptr[3], 2);
    memcpy((void*)&count, &lptr[5], 4);

    return true;
  }
};
struct ApiVersionResult {
  bool ok;
  short status;
  int version;
};

struct Ticket {
  uint32_t GlobalID;
  uint32_t SourceID;
  uint32_t DestinationID;
  uint32_t TicketID;
  uint32_t ResponseID = UINT_MAX;
  uint32_t data_buf_len;
  uint32_t date_buf_len;
  char* Data;
  char* Date;

  void _free() {
    if (Data) safe_free(Data);
    if (Date) safe_free(Date);
  }
  ~Ticket() {
    _free();
  }
};
struct TicketResult {
  uint16_t cid = TicketResult_CID;
  bool ok;
  int16_t status;
  uint8_t t_count;
  Ticket* tickets;

  void _free() {
    for (int x = 0; x < this->t_count; x++) {
      this->tickets[x]._free();
    }
    if (this->tickets) safe_free(this->tickets);

    this->t_count = 0;
  }
  TicketResult() {};
  TicketResult(short ticketsCount) {
    tickets = (Ticket*)safe_alloc(sizeof(Ticket) * ticketsCount);
    t_count = ticketsCount;
  }
  ~TicketResult() {
    _free();
  }

  RawDataPacket to_packet() {
    int size = TicketResult_SIZE;
    for (int i = 0; i < t_count; i++) {
      size += Ticket_SIZE + tickets[i].data_buf_len + tickets[i].date_buf_len;
    }
    RawDataPacket p = RawDataPacket(size);

    uint8_t* lptr = (uint8_t*)p.buf;
    memcpy(&lptr[0], (void*)&cid, 2);
    memcpy(&lptr[2], (void*)&ok, 1);
    memcpy(&lptr[3], (void*)&status, 2);
    memcpy(&lptr[5], (void*)&t_count, 1);

    int offset = 6;
    for (int i = 0; i < t_count; i++) {
      memcpy(&lptr[offset], (void*)&tickets[i].GlobalID, 4);
      memcpy(&lptr[offset + 4], (void*)&tickets[i].SourceID, 4);
      memcpy(&lptr[offset + 8], (void*)&tickets[i].DestinationID, 4);
      memcpy(&lptr[offset + 12], (void*)&tickets[i].TicketID, 4);
      memcpy(&lptr[offset + 16], (void*)&tickets[i].ResponseID, 4);
      memcpy(&lptr[offset + 20], (void*)&tickets[i].data_buf_len, 4);
      memcpy(&lptr[offset + 24], (void*)&tickets[i].date_buf_len, 4);

      memcpy(&lptr[offset + 28], (void*)tickets[i].Data, tickets[i].data_buf_len);
      memcpy(&lptr[offset + 28 + tickets[i].data_buf_len], (void*)tickets[i].Date, tickets[i].date_buf_len);

      offset += Ticket_SIZE + tickets[i].data_buf_len + tickets[i].date_buf_len;
    }

    return p;
  }
  bool from_packet(RawDataPacket p) {
    if (p.buf_size < TicketResult_SIZE) return false;

    uint8_t* lptr = (uint8_t*)(p.buf);
    ushort _cid = 0;
    memcpy((void*)&_cid, &lptr[0], 2);
    if (_cid != TicketResult_CID) return false;
    cid = _cid;

    memcpy((void*)&ok, &lptr[2], 1);
    memcpy((void*)&status, &lptr[3], 2);
    memcpy((void*)&t_count, &lptr[5], 1);

    tickets = (Ticket*)safe_alloc(sizeof(Ticket) * t_count);
    int offset = 6;
    for (int i = 0; i < t_count; i++) {
      memcpy((void*)&tickets[i].GlobalID, &lptr[offset], 4);
      memcpy((void*)&tickets[i].SourceID, &lptr[offset + 4], 4);
      memcpy((void*)&tickets[i].DestinationID, &lptr[offset + 8], 4);
      memcpy((void*)&tickets[i].TicketID, &lptr[offset + 12], 4);
      memcpy((void*)&tickets[i].ResponseID, &lptr[offset + 16], 4);
      memcpy((void*)&tickets[i].data_buf_len, &lptr[offset + 20], 4);
      memcpy((void*)&tickets[i].date_buf_len, &lptr[offset + 24], 4);

      tickets[i].Data = (char*)malloc(tickets[i].data_buf_len + 1);
      tickets[i].Date = (char*)malloc(tickets[i].date_buf_len + 1);

      tickets[i].Data[tickets[i].data_buf_len] = 0;
      tickets[i].Date[tickets[i].date_buf_len] = 0;

      memcpy((void*)tickets[i].Data, &lptr[offset + 28], tickets[i].data_buf_len);
      memcpy((void*)tickets[i].Date, &lptr[offset + 28 + tickets[i].data_buf_len], tickets[i].date_buf_len);

      offset += Ticket_SIZE + tickets[i].data_buf_len + tickets[i].date_buf_len;

      if ((uint32_t)offset > p.buf_size) {
        for (int x = 0; x < i + 1; x++) tickets[x]._free();
        delete tickets;
        return false;
      }
    }

    return true;
  }
};

struct SetValue {
  const uint16_t cid = SetValue_CID;

  // TODO
};
struct GetValue {
  const uint16_t cid = GetValue_CID;

  // TODO
};
struct GetSchema {
  const uint16_t cid = GetSchema_CID;

  // TODO
};
struct SHProto_Ping {
  // TODO
};


bool Init();
const char* GetHash();
bool IsConnected();
bool sync(int write);
void SetTicketCallback(void (*callback)(TicketResult t));
bool Tick();
bool Ping();
void sig_TCP_down();

bool Auth(Credentials credits, SecureResult* res);

//SecureResult UpdatePass(String newpassword);
//SecureResult Create(Credentials credits);
//SecureResult Delete(Credentials credits);

//TicketResult TicketPush(Ticket ticket);
//TicketResult TicketPull(uint offset = UINT_MAX, short count = SHRT_MAX);
//TicketServiceResult TicketGetLast();
//TicketServiceResult TicketFlush();

#endif