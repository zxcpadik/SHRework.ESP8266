#ifndef __TCPPROTO__
#define __TCPPROTO__

#include <Arduino.h>
#include <climits>
#include <ESP8266WiFi.h>
#include "../system/safemem.h"

#define HOST_MAIN "sh-rework.ru"
#define HOST_BACKUP "192.168.1.105"
#define PORT 8090
#define APP_HASH "58e57fa170955c5f"
#define APP_HASH_SIZE 8
#define SESSION_HASH_SIZE 32
#define PROTO_GUARD "SHRework"

#define MAX_PACKET_SIZE 512

#define SIG_SLAVE_VOID_READ  100777100
#define SIG_SLAVE_VOID_WRITE 200777200
#define SIG_SLAVE_DATA_READ  300777300
#define SIG_SLAVE_DATA_WRITE 400777400
#define SIG_MASTER_VOID_READ  100555100
#define SIG_MASTER_VOID_WRITE 200555200
#define SIG_MASTER_DATA_READ  300555300
#define SIG_MASTER_DATA_WRITE 400555400
#define SIG_CROSS  100999100
#define SIG_CANCEL 200999200


#define SecureResult_SIZE 9
#define SecureResult_CID 13

#define Credentials_SIZE 4
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
  size_t buf_size = 0;
  void* buf = nullptr;

  int GetSize() {
    return buf_size + 4;
  }
  ushort getcid() {
    ushort ret = 0;
    uint8_t* lptr = (uint8_t*)buf;
    if (buf_size >= 2) memcpy(&ret, &lptr[0], 2);
    return ret;
  }
  bool getok() {
    bool ret = false;
    uint8_t* lptr = (uint8_t*)buf;
    if (buf_size >= 3) memcpy(&ret, &lptr[2], 1);
    return ret;
  }
  short getstatus() {
    short ret = 0;
    uint8_t* lptr = (uint8_t*)buf;
    if (buf_size >= 5) memcpy(&ret, &lptr[3], 2);
    return ret;
  }

  void _free() {
    my_safe_free(buf);
    buf_size = 0;
  }
  void _alloc(size_t size) {
    buf = my_safe_alloc(size, 0);
    buf_size = size;
  }
  RawDataPacket() {
    buf_size = 0;
    buf = nullptr;
  }
  RawDataPacket(size_t size) {
    _alloc(size);
  }
  ~RawDataPacket() {
    _free();
  }
};

struct SecureResult {
  uint16_t cid = SecureResult_CID;
  bool ok;
  int16_t status;
  int32_t userid;

  void to_packet(RawDataPacket& p) {
    p._alloc(SecureResult_SIZE);

    uint8_t* lptr = (uint8_t*)p.buf;
    memcpy(&lptr[0], (void*)&cid, 2);
    memcpy(&lptr[2], (void*)&ok, 1);
    memcpy(&lptr[3], (void*)&status, 2);
    memcpy(&lptr[5], (void*)&userid, 4);
  }
  bool from_packet(RawDataPacket& p) {
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
  uint8_t username_len;
  uint8_t password_len;
  char username[256];
  char password[256];

  String getUsername() {
    return String(username);
  }
  String getPassword() {
    return String(username);
  }

  Credentials() {}
  Credentials(String _username, String _password) {
    strcpy(this->username, _username.c_str());
    strcpy(this->password, _password.c_str());

    this->username_len = strlen(this->username);
    this->password_len = strlen(this->password);
  }
  Credentials(const char* _username, const char* _password) {
    strcpy(this->username, _username);
    strcpy(this->password, _password);

    this->username_len = strlen(_username);
    this->password_len = strlen(_password);
  }

  size_t getUsernameLen() { return strlen(this->username); }
  size_t getPasswordLen() { return strlen(this->password); }

  void to_packet(RawDataPacket& p) {
    size_t ulen = this->getUsernameLen();
    size_t plen = this->getPasswordLen();

    int size = Credentials_SIZE + ulen + plen;
    p._alloc(size);

    uint8_t* lptr = (uint8_t*)p.buf;
    memcpy(&lptr[0], (void*)&cid, 2);
    lptr[2] = ulen;
    lptr[3] = plen;
    memcpy(&lptr[4], (void*)username, ulen);
    memcpy(&lptr[4 + ulen], (void*)password, plen);
  }
  bool from_packet(RawDataPacket& p) {
    if (p.buf_size != Credentials_SIZE) return false;

    uint8_t* lptr = (uint8_t*)p.buf;
    ushort _cid = 0;
    memcpy((void*)&_cid, &lptr[0], 2);
    if (_cid != Credentials_CID) return false;
    cid = _cid;

    this->username_len = lptr[2];
    this->password_len = lptr[3];

    memset(this->username, 0, sizeof(this->username));
    memset(this->password, 0, sizeof(this->password));

    memcpy(this->username, &lptr[4], this->username_len);
    memcpy(this->password, &lptr[4 + this->username_len], this->password_len);
    return true;
  }
};
struct TicketServiceResult {
  uint16_t cid = Credentials_CID;
  bool ok;
  int16_t status;
  int32_t count;

  RawDataPacket to_packet() {
    RawDataPacket p;
    p._alloc(SecureResult_SIZE);

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
    if (Data) my_safe_free(Data);
    if (Date) my_safe_free(Date);
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
    my_safe_free(this->tickets);
    this->t_count = 0;
  }
  TicketResult() {};
  TicketResult(short ticketsCount) {
    tickets = (Ticket*)my_safe_alloc(sizeof(Ticket) * ticketsCount, 3);
    this->t_count = ticketsCount;
  }
  ~TicketResult() {
    this->_free();
  }

  void to_packet(RawDataPacket& p) {
    int size = TicketResult_SIZE;
    for (int i = 0; i < t_count; i++) {
      size += Ticket_SIZE + tickets[i].data_buf_len + tickets[i].date_buf_len;
    }
    p._alloc(size);

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
  }
  bool from_packet(RawDataPacket& p) {
    if (p.buf_size < TicketResult_SIZE) return false;

    uint8_t* lptr = (uint8_t*)(p.buf);
    ushort _cid = 0;
    memcpy((void*)&_cid, &lptr[0], 2);
    if (_cid != TicketResult_CID) return false;
    cid = _cid;

    memcpy((void*)&ok, &lptr[2], 1);
    memcpy((void*)&status, &lptr[3], 2);
    memcpy((void*)&t_count, &lptr[5], 1);

    tickets = (Ticket*)my_safe_alloc(sizeof(Ticket) * t_count, 4);
    int offset = 6;
    for (int i = 0; i < t_count; i++) {
      memcpy((void*)&tickets[i].GlobalID, &lptr[offset], 4);
      memcpy((void*)&tickets[i].SourceID, &lptr[offset + 4], 4);
      memcpy((void*)&tickets[i].DestinationID, &lptr[offset + 8], 4);
      memcpy((void*)&tickets[i].TicketID, &lptr[offset + 12], 4);
      memcpy((void*)&tickets[i].ResponseID, &lptr[offset + 16], 4);
      memcpy((void*)&tickets[i].data_buf_len, &lptr[offset + 20], 4);
      memcpy((void*)&tickets[i].date_buf_len, &lptr[offset + 24], 4);

      tickets[i].Data = (char*)my_safe_alloc(tickets[i].data_buf_len + 1);
      tickets[i].Date = (char*)my_safe_alloc(tickets[i].date_buf_len + 1);

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
bool sync(bool _write);
void SetTicketCallback(void (*callback)(TicketResult& t));
bool Tick();
bool Ping();
void sig_TCP_down();

WiFiClient getClient();

bool Auth(Credentials& credits, SecureResult& res);

//SecureResult UpdatePass(String newpassword);
//SecureResult Create(Credentials credits);
//SecureResult Delete(Credentials credits);

//TicketResult TicketPush(Ticket ticket);
//TicketResult TicketPull(uint offset = UINT_MAX, short count = SHRT_MAX);
//TicketServiceResult TicketGetLast();
//TicketServiceResult TicketFlush();

#endif