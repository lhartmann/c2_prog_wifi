#include <httpServer.h>
#include <httpServerPage.h>
#include <c2.h>
#ifdef ARDUINO_ARCH_ESP32
#include <WiFi.h>
#include <ESPmDNS.h>
#else
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#endif
#include <WiFiClient.h>
#include <ctype.h>
#include "serialServer.h"
#include "ihx.h"

#define WIFI_NAME     ""
#define WIFI_PASSWORD ""

const char *c2_print_status_by_name(uint8_t ch) {
  switch (ch) {
    case C2_SUCCESS:       return "Success";
    case C2_SHIFT_TIMEOUT: return "Shift wait timeout error";
    case C2_POLL_TIMEOUT:  return "Register poll timeout error";
    case C2_CMD_ERROR:     return "In-command error";
    case C2_BROKEN_LINK:   return "Broken link, address read failed";
    default:               return "unknown error";
  }
}

//////////////////////
// Hello World Page //
//////////////////////
class HelloWorldPage : public HttpServerPage {
    void onEndOfHeader() {
      c->write("<!DOCTYPE html>"
               "<html>"
               "<body>"
               "<form action=\"upload\" method=\"post\" enctype=\"multipart/form-data\">"
               "Select image to upload:"
               "<input type=\"file\" name=\"fileToUpload\" id=\"fileToUpload\">"
               "<input type=\"submit\" value=\"Upload Image\" name=\"submit\">"
               "</form>"
               "</body>"
               "</html>");
      c->stop();
    }
} helloWorldPage;

//////////////////////
// Error Page       //
//////////////////////
class ErrorPage : public HttpServerPage {
    void onEndOfHeader() {
      c->write("HTTP/1.1 404 Not Found\r\n\r\n");
      c->write("<h1>Page Not Found.</h1>");
      c->stop();
    }
} errorPage;

class EchoPage : public HttpServerPage {
    bool headerprinted;
    void onConnect(WiFiClient &client, const char *sub, const char *hash) {
      HttpServerPage::onConnect(client,sub,hash);
      c->print("HTTP/1.1 200 OK\r\n");
      c->print("Content-Type: text/html\r\n\r\n");
      c->print("Subpath: ");
      c->print("sub");
      c->print("Hash tag: ");
      c->println(hash);
      headerprinted = false;
    }
    void onQueryVar(char *var, char *value) {
      HttpServerPage::onQueryVar(var, value);
      c->print(var);
      c->print(" = ");
      c->print(value);
      c->print("<br />");
    }
    void onHeader(char *str) {
      HttpServerPage::onHeader(str);
      if (!headerprinted) c->print("<h1>Echo Page</h1><h2>Headers:</h2><pre>");
      headerprinted = true;
      c->println(str);
    }
    void onEndOfHeader() {
      //      HttpServerPage::onEndOfHeader();
      c->print("</pre><h2>Body</h2><pre>");
    }
    int onData(char *buff, int count) {
      HttpServerPage::onData(buff, count);
      c->println(buff);
      return count;
    }
} echoPage;

//////////////////////
// Upload Page      //
//////////////////////
class UploadPage : public HttpServerPage {
    bool headerprinted;
    uint8_t devid;
    uint8_t revid;
    void onConnect(WiFiClient &client, const char *sub, const char *hash) {
      HttpServerPage::onConnect(client,sub,hash);
      c->print("HTTP/1.1 200 OK\r\n");
      c->print("Content-Type: text/html, charset=utf8\r\n\r\n");
      c->println("<h1>Lucas Hartmann's EFM8BB1 HTTP Programmer</h1>");
      headerprinted = false;
    }
    void onEndOfHeader() {
      uint8_t err;
      c->print("<pre>Enable programming mode: ");
      err = c2_programming_init(C2_DEVID_UNKNOWN);
      c->println(c2_print_status_by_name(err));
      if (err != C2_SUCCESS) {
        c->stop();
        return;
      }

      c->print("Device ID: ");
      err = c2_programming_init(C2_DEVID_UNKNOWN);
      c2_address_write(C2DEVID);
      err = c2_data_read(devid, 1);
      if (err != C2_SUCCESS) {
        c->println(c2_print_status_by_name(err));
        c->stop();
        return;
      }
      c->print  ("0x");
      c->println(devid, HEX);

      c->print("Revision ID: ");
      err = c2_programming_init(C2_DEVID_UNKNOWN);
      c2_address_write(C2REVID);
      err = c2_data_read(revid, 1);
      if (err != C2_SUCCESS) {
        c->println(c2_print_status_by_name(err));
        c->stop();
        return;
      }
      c->print  ("0x");
      c->println(revid, HEX);

      c->print("Erase flash: ");
      err = c2_programming_init(devid);
      err = c2_device_erase();
      c->println(c2_print_status_by_name(err));
      if (err != C2_SUCCESS) {
        c->stop();
        return;
      }
    }
    int onData(char *buff, int count) {
      HttpServerPage::onData(buff, count);

      uint8_t err = ihx_decode((uint8_t*)buff);
      if (err != IHX_SUCCESS) return count;

      ihx_t *h = (ihx_t*)buff;
      uint16_t address = h->address_high * 0x100 + h->address_low;
/*      c->print("Packet: type[");
      c->print(h->record_type, HEX);
      c->print("] address[");
      c->print(address, HEX);
      c->print("] len[");
      c->print(h->len, HEX);
      c->println("]"); */

      // Write data records
      if (h->record_type == IHX_RT_DATA) {
        c->print  ("Programming 0x");
        c->print  (address, HEX);
        c->print  (": ");
        int retries = 5;
        do {
          err = c2_programming_init(devid);
          err = c2_block_write(address, h->data, h->len);
        } while (err != C2_SUCCESS && retries--);
        c->println(c2_print_status_by_name(err));
      }

      // Reset on end-of-file
      if (h->record_type == IHX_RT_END_OF_FILE) {
        c->println("End of file, resetting target.");
        err = c2_reset();
        c->println(c2_print_status_by_name(err));
        c->stop();
      }

      return count;
    }
} uploadPage;

HttpServer::PageMap pageMap[] = {
  { "/", &helloWorldPage },
  { "/upload", &uploadPage },
  { "/echo", &echoPage },
  { 0, &errorPage } // End of list and Page-Not-Found handler
};

// Looper
uint8_t i, ch;

// Servers
HttpServer httpServer(pageMap);

class EFM8HttpBurner_t {
    // Intel Hex data buffer
    union {
      uint8_t b[256];
      ihx_t h;
    } buff;
};

// Setup
MDNSResponder mdns;

void setup() {
  // put your setup code here, to run once:
  pinMode(PIN_C2CK, OUTPUT);
  pinMode(PIN_C2D,  INPUT);

  Serial.begin(115200);

  // Wifi Setup
  WiFi.begin(WIFI_NAME, WIFI_PASSWORD);
}

bool online = false;
void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    if (online) {
      httpServer.stop();
      //      serialServer.stop();
      online = false;
    }
  } else {
    if (!online) {
      httpServer.begin();
      //      serialServer.begin(115200);
      online = true;
#ifdef ARDUINO_ARCH_ESP32
      mdns.begin("esp32");
#else
      mdns.begin("esp8266", WiFi.localIP(), 5*60);
#endif
    }
    httpServer.loop();
    serialServer.loop();
#ifndef ARDUINO_ARCH_ESP32
    mdns.update();
#endif
  }
}

// This is to enforce arduino-like formatting in kate
// kate: space-indent on; indent-width 2; mixed-indent off; indent-mode cstyle;
