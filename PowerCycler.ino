#include <SPI.h>
#include <util.h>
#include <EthernetUdp.h>
#include <EthernetServer.h>
#include <Ethernet.h>

#define MAX_STATS 8

const int powerPin = 7;
unsigned long rebootTime;
int rebootCount = 0;

byte mac[] = {
	0x90, 0xA2, 0xDA, 0x00, 0xCB, 0x86 };
IPAddress ip(192, 168, 1, 220);

unsigned int localPort = 6666;      // local port to listen on

// An EthernetUDP instance to let us send and receive packets over UDP
EthernetUDP Udp;
EthernetServer webServer(80);

char packetBuffer[UDP_TX_PACKET_MAX_SIZE]; //buffer to hold incoming packet,

typedef struct {
	uint32_t address;
	int lastReboot;
} rebootStat;

rebootStat stats[MAX_STATS];
int numStats = 0;

void setup() {
	pinMode(powerPin, OUTPUT);
	digitalWrite(powerPin, HIGH);
	// start the Ethernet and UDP:
	Ethernet.begin(mac, ip);

	webServer.begin();

	Udp.begin(localPort);
	Serial.begin(9600);
	rebootTime = millis();
}

void loop() {
	EthernetClient client = webServer.available();
	if (client) {
		handleClient(client);
	}

	handleUDPPing();
}

void handleClient(EthernetClient client) {
	//Serial.println("new client");
	// an http request ends with a blank line
	boolean currentLineIsBlank = true;
	while (client.connected()) {
		if (client.available()) {
			char c = client.read();
			//Serial.write(c);
			// if you've gotten to the end of the line (received a newline
			// character) and the line is blank, the http request has ended,
			// so you can send a reply
			if (c == '\n' && currentLineIsBlank) {
				// send a standard http response header
				client.println("HTTP/1.1 200 OK");
				client.println("Content-Type: text/html");
				client.println("Connection: close");  // the connection will be closed after completion of the response
				client.println("Refresh: 10");  // refresh the page automatically every 5 sec
				client.println();
				client.println("<!DOCTYPE HTML>");
				client.println("<html>");
				client.print("Total reboots: ");
				client.print(rebootCount, DEC);
				client.print("<br>Total clients: ");
				client.print(numStats, DEC);
				client.println("<br><br>");
				// output the value of each analog input pin
				for (int i = 0; i < numStats; i++) {
					for (int j = 0; j < 4; j++)
					{
						client.print(((uint8_t*)&(stats[i].address))[j], DEC);
						if (j < 3)
						{
							client.print(".");
						}
					}
					//client.print(stats[i].address, DEC);
					client.print(" Last seen at reboot: ");
					client.print(stats[i].lastReboot, DEC);
					client.println("<br />");
				}
				client.println("</html>");
				break;
			}
			if (c == '\n') {
				// you're starting a new line
				currentLineIsBlank = true;
			}
			else if (c != '\r') {
				// you've gotten a character on the current line
				currentLineIsBlank = false;
			}
		}
	}
	// give the web browser time to receive the data
	delay(20);
	// close the connection:
	client.stop();
	//Serial.println("client disconnected");
}

void handleUDPPing() {
	int packetSize = Udp.parsePacket();
	if (packetSize)
	{
		Serial.print("Received packet of size ");
		Serial.println(packetSize);
		Serial.print("From ");
		IPAddress remote = Udp.remoteIP();
		for (int i = 0; i < 4; i++)
		{
			Serial.print(remote[i], DEC);
			if (i < 3)
			{
				Serial.print(".");
			}
		}
		Serial.print(", port ");
		Serial.println(Udp.remotePort());

		bool foundEntry = false;
		for (int i = 0; i < numStats; i++) {
			if (stats[i].address == remote) {
				stats[i].lastReboot = rebootCount;
				foundEntry = true;
			}
		}
		if (!foundEntry && numStats < MAX_STATS) {
			stats[numStats].address = remote;
			stats[numStats].lastReboot = rebootCount;
			numStats++;
		}

		// read the packet into packetBufffer
		Udp.read(packetBuffer, UDP_TX_PACKET_MAX_SIZE);
		//Serial.println("Contents:");
		//Serial.println(packetBuffer);
	}
	if ((millis() - rebootTime) > 60 * (long)1000) {
		togglePower();
	}
}

void togglePower() {
	digitalWrite(powerPin, LOW);
	delay(1000);
	digitalWrite(powerPin, HIGH);
	rebootCount += 1;
	Serial.print("Reboot Count: ");
	Serial.println(rebootCount);
	rebootTime = millis();
}