/**************************************************************************
 * (C)2015 Lucas V. Hartmann <lhartmann@github.com>                       *
 *                                                                        *
 * This program is free software: you can redistribute it and/or modify   *
 * it under the terms of the GNU General Public License as published by   *
 * the Free Software Foundation, either version 3 of the License, or      *
 * (at your option) any later version.                                    *
 *                                                                        *
 * This program is distributed in the hope that it will be useful,        *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 * GNU General Public License for more details.                           *
 *                                                                        *
 * You should have received a copy of the GNU General Public License      *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.  *
 *************************************************************************/

#include <httpServer.h>
#include <httpServerPage.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiClient.h>
#include <ctype.h>

#define WIFI_NAME     ""
#define WIFI_PASSWORD ""

//////////////////////
// Hello World Page //
//////////////////////
class HelloWorldPage : public HttpServerPage {
    void onEndOfHeader() {
      c->write("<!DOCTYPE html>\
<html>\
<body>\
\
<form action=\"upload\" method=\"post\" enctype=\"multipart/form-data\">\
    Select image to upload:\
    <input type=\"file\" name=\"fileToUpload\" id=\"fileToUpload\">\
    <input type=\"submit\" value=\"Upload Image\" name=\"submit\">\
</form>\
\
</body>\
</html>");
      c->stop();
    }
} helloWorldPage;

////////////////////////////////////////////////////////////////////////////
// Error Page                                                             //
//   You may choose to look for files in filesystem instead of just       //
//   printing an error message. This would allow for a file-based server. //
////////////////////////////////////////////////////////////////////////////
class ErrorPage : public HttpServerPage {
    void onEndOfHeader() {
      c->write("HTTP/1.1 404 Not Found\r\n\r\n");
      c->write("<h1>Page Not Found.</h1>");
      c->stop();
    }
} errorPage;

/////////////////////
// Echo Page       //
/////////////////////
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

//////////////
// Page Map //
//////////////
HttpServer::PageMap pageMap[] = {
  { "/", &helloWorldPage },
  { "/echo", &echoPage },
  { 0, &errorPage } // End of list and Page-Not-Found handler
};

// Servers
HttpServer httpServer(pageMap);

// Setup
MDNSResponder mdns;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  // Wifi Setup
  WiFi.begin(WIFI_NAME, WIFI_PASSWORD);
}

bool online = false;
void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    if (online) {
      httpServer.stop();
      online = false;
    }
  } else {
    if (!online) {
      // Network reconnect
      online = true;
      
      // Reconfigure HTTP server
      httpServer.begin();

      // Allows http://esp8266.local to be used instead of IP.
      mdns.begin("esp8266", WiFi.localIP(), 5*60);
    }
    
    // Network is online, do the polling
    httpServer.loop();
    mdns.update();
  }
}

// This is to enforce arduino-like formatting in kate-editor.
// kate: space-indent on; indent-width 2; mixed-indent off; indent-mode cstyle;
