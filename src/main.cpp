#include "Arduino.h"
#include "proto.h"

#define PIN_R D1
#define PIN_G D2
#define PIN_B D5

#define SERVICE_USERNAME "ledstrip_0afb8683"
#define SERVICE_PASSWORD "bSPS81ePSADMMGVhQhMg"
#define WIFI_SSID "Barsik_2.4G"
#define WIFI_PASS "Home4444"

const unsigned long long SERIAL_NUMBER = 12614259606799524000ULL;
const char* GLOBAL_ID = "0afb868351d8d904";

JsonDocument JSON;
Credentials credits = Credentials{ .username = SERVICE_USERNAME, .password = SERVICE_PASSWORD };
uint lastID = 1;

struct Color {
	uint8_t R;
	uint8_t G;
	uint8_t B;

	bool operator==(const Color& color) const {
		return (R == color.R) && (G == color.G) && (B == color.B);
	}
};

Color smoothColor(Color current, Color target);
void writeColor(Color& color);
void Panic();
void ParseTicket(Ticket ticket);

Color red = { 255, 0, 0 };
Color green = { 0, 255, 0 };
Color blue = { 0, 0, 255 };
int cur = 1;

Color current = red;
Color target = red;;

unsigned long timer1_timeout = 3000;
unsigned long timer1_last = 0;

unsigned long timer2_timeout = 15;
unsigned long timer2_last = 0;

unsigned long timer3_timeout = 15;
unsigned long timer3_last = 0;

void loop() {
	if (millis() - timer2_last > timer2_timeout) {
		timer2_last = millis();
		writeColor(current);
	}

	if (millis() - timer3_last > timer3_timeout) {
		timer3_last = millis();
		current = smoothColor(current, target);
	}

	if (millis() - timer1_last > timer1_timeout) {
		timer1_last = millis();

		TicketResult result = TicketPull(credits, lastID, 1);
		if (result.t_count > 0) {
			lastID += result.t_count;

			for (ushort i = 0; i < result.t_count; i++) {
				ParseTicket(result.tickets[i]);
			}
		}
		delete result.tickets;
	}
}

void setup() {
	//Serial.begin(9600);
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

	SecureResult createres = Create(credits);
	if (createres.status != 120 && createres.status != 122) Panic();

	TicketServiceResult tlastres = TicketGetLast(credits);
	if (!tlastres.ok) Panic();
	lastID = tlastres.count;
	//Serial.print("LAST ID: ");
	//Serial.println(String(lastID));

	pinMode(PIN_R, OUTPUT);
	pinMode(PIN_G, OUTPUT);
	pinMode(PIN_B, OUTPUT);
}

void writeColor(Color& color) {
	analogWrite(PIN_R, 255 - color.R % 256);
	analogWrite(PIN_G, 255 - color.G % 256);
	analogWrite(PIN_B, 255 - color.B % 256);
}

Color smoothColor(Color current, Color target) {
	Color cur = current;
	cur.R += (uint8_t)ceil(((float)target.R - (float)current.R) / (float)10);
	cur.G += (uint8_t)ceil(((float)target.G - (float)current.G) / (float)10);
	cur.B += (uint8_t)ceil(((float)target.B - (float)current.B) / (float)10);
	return cur;
}

void Panic() {
	bool status = false;
	for (int i = 0; i < 100; i++) {
		digitalWrite(LED_BUILTIN, status);
		status = !status;
		delay(200);
	}
	ESP.restart();
}

void ParseTicket(Ticket ticket) {
	String data = String(ticket.Data);
	int index = data.indexOf(':');
	String cmd = data.substring(0, index);
	String value = data.substring(index + 1);

	if (cmd == "led") {
		int state = value.toInt();
		digitalWrite(LED_BUILTIN, !state);
	} else if (cmd == "set") {
		int sp1index = value.indexOf(':');
		int sp2index = value.indexOf(':', sp1index + 1);

		uint8_t R = value.substring(0, sp1index).toInt();
		uint8_t G = value.substring(sp1index + 1, sp2index).toInt();
		uint8_t B = value.substring(sp2index + 1).toInt();

		//Serial.println("DATA!");
		//Serial.println(value.substring(0, sp1index));
		//Serial.println(value.substring(sp1index + 1, sp2index));
		//Serial.println(value.substring(sp2index + 1));


		target = Color{ R, G, B };
	} else if (cmd == "get") {
		JSON["R"] = current.R;
		JSON["G"] = current.G;
		JSON["B"] = current.B;

		String data;
		serializeJson(JSON, data);

		Ticket res = Ticket();
		res.DestinationID = ticket.SourceID;
		res.Data = data.c_str();
		res.ResponseID = ticket.ResponseID;
		TicketResult tres = TicketPush(credits, res);
		delete tres.tickets;
		data.clear();
	}
}

