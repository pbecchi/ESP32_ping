/*
 Name:		PingTest.ino
 Created:	05/08/2017 10:20:19
 Author:	pbecc
*/
#undef PROVA_


#include <WiFi.h>
#include  "ping.h"

// the setup function runs once when you press reset or power the board
const char ssid[] = "TP-LINK_C20B";  //  your network SSID (name)
const char password[] = "paolo-48";       // your network password

const char *host = "www.google.com"; // ping target hostname

void setup() {

	Serial.begin(115200);
	Serial.print("Connecting t ");
	Serial.println(ssid);
	// WiFi.mode(WIFI_STA);

	WiFi.begin(ssid, password);


	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		Serial.print(".");
		

	}
	Serial.println("Pinging address: 192.168.1.1");
}

// the loop function runs over and over again until power down or reset
void loop() {
	int ia[4] = { 192,168,1,1 };
	int  i = 0;
	while (Serial.available()) {
		
		char c = Serial.read();
		delay(100);
   	int  val = 0;
		while (c != '.' &&  c != 10 && c!=255) {
			if (c >= '0'&& c<='9') {
				val = val*10+(c-'0');				
			}
			c = Serial.read();
		}
	
			ia[i++] =val ;
	}
	IPAddress adr = IPAddress(ia[0], ia[1], ia[2], ia[3]);
	Serial.printf("Ping : %d . %d . %d . %d -> ", ia[0], ia[1], ia[2], ia[3]);
	struct ping_result result;
	if (ping_start(adr, 4, 0, 0, 5, &result)) {
		Serial.println("OK");
		Serial.println("--- statistics ---");
		Serial.print(result.transmitted);
		Serial.print(" packets transmitted, ");
		Serial.print(result.received);
		Serial.print(" packets received, ");
		Serial.print(result.loss_rate, 2);
		Serial.println("% packet loss");
		Serial.print("round-trip min/avg/max/stddev = ");
		Serial.print(result.min_time, 3);
		Serial.print("/");
		Serial.print(result.mean_time, 3);
		Serial.print("/");
		Serial.print(result.max_time, 3);
		Serial.print("/");
		Serial.print(result.var_time / result.received, 3);
		Serial.println(" ms");
	} else {
		Serial.println("FAILED");
		Serial.println("--- statistics ---");
		Serial.print(result.transmitted);
		Serial.print(" packets transmitted, ");
		Serial.print(result.received);
		Serial.print(" packets received, ");
		Serial.print(result.loss_rate, 2);
		Serial.println("% packet loss");
	}

	Serial.printf("Ping : %s -> ", host);
 	if (ping(host, 4, 0, 0, 5, &result)) {
		Serial.println("OK");
		Serial.println("--- statistics ---");
		Serial.print(result.transmitted);
		Serial.print(" packets transmitted, ");
		Serial.print(result.received);
		Serial.print(" packets received, ");
		Serial.print(result.loss_rate, 2);
		Serial.println("% packet loss");
		Serial.print("round-trip min/avg/max/stddev = ");
		Serial.print(result.min_time, 3);
		Serial.print("/");
		Serial.print(result.mean_time, 3);
		Serial.print("/");
		Serial.print(result.max_time, 3);
		Serial.print("/");
		Serial.print(result.var_time / result.received, 3);
		Serial.println(" ms");
	} else {
		Serial.println("FAILED");
		Serial.println("--- statistics ---");
		Serial.print(result.transmitted);
		Serial.print(" packets transmitted, ");
		Serial.print(result.received);
		Serial.print(" packets received, ");
		Serial.print(result.loss_rate, 2);
		Serial.println("% packet loss");
	 }
	delay(5000);

}
int readnu(char s) {
	char c = Serial.read();
	Serial.print(c);
	int digit = 1,val=0;
	while (c != s &&  c != 10&&c>0) {
		if(c>'0') val += digit*(c - '0');
		digit *= 10;
		c=Serial.read();
		Serial.print(int(c));
	}
	Serial.println(digit);
	return digit;
}
