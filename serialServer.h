#ifndef SERIALSERVER_H
#define SERIALSERVER_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>

struct SerialServer {
	WiFiServer s;
	WiFiClient c;
	
	SerialServer(int port) : s(port) {};
	
	void begin(int baud);
	void stop();
	void loop();
	
} serialServer(9600);

void SerialServer::begin(int baud) {
	s.begin();
	Serial.begin(baud);
}

void SerialServer::stop() {
	c.stop();
//	s.end();
	Serial.end();
}

void SerialServer::loop() {
	// Handle disconnects
	if (!c.connected()) {
		c.stop();
	}
	
	// Accept incoming connections
	if (!c) {
		c = s.available();
	}
	
	// Handle communicatons
	if (c) {
		size_t bytes;
		int maxbytes = 128;
		uint8_t buff[maxbytes];

		// Network to serial
		bytes = c.available();
		if (bytes) {
			if (bytes > maxbytes) bytes = maxbytes;
			c.read(buff, bytes);
			Serial.write(buff, bytes);
			Serial.write((char*)0, 0);
		}

		// Serial to network
		bytes = 0;
		while (Serial.available() && bytes < maxbytes) {
			buff[bytes++] = Serial.read();
		}
		c.write(buff,bytes);
	}
}

#endif

// This is to enforce arduino-like formatting in kate
// kate: space-indent on; indent-width 2; mixed-indent off; indent-mode cstyle;
