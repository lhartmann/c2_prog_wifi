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

#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include "string.h"
#include <httpServerPage.h>

#define DEBUG(x) (x)
#define DEBUG_DUMP(x) { Serial.print( #x " = " ); Serial.println(x); }

struct HttpServer {
  WiFiServer s;
  WiFiClient c;
  char buff[256];
  uint8_t bp;

  const struct PageMap {
    const char *target;
    HttpServerPage *page;
  } *pages;
  HttpServerPage *page;

  enum {
    stRequest, // First line, HTTP request "GET / HTTP/1.1"
    stHeaders, // HTTP Headers "Content-Type: crazy/stuff"
    stData     // Data segment
  } state;

  HttpServer(const PageMap *pm) : s(80), pages(pm) {}
  void setPageMap(const PageMap *pm) { pages = pm; }

  void begin();
  void stop();
  void loop();

  // Auxiliary function blanks removal
  static char *clean(char *p);
  static void urldecode(const char *in, char *out);

  // Handlers
  void handleRequest(char *str);
};

void HttpServer::begin() {
  s.begin();
  page = 0;
  bp = 0;
}

void HttpServer::stop() {
  c.stop();
  //	s.end();
}

void HttpServer::loop() {
  char *str;

  // Handle disconnects
  if (c && !c.connected()) {
    DEBUG(Serial.println("Client disconnected."));
    if (page) page->onClose();
    c.stop();
  }

  // Accept incoming connections
  if (!c) {
    c = s.available();
    state = stRequest;
    if (c) DEBUG(Serial.println("Client connected."));
    bp = 0;
  }

  // Handle communicatons
  if (!c) return;

  char ch;
  do {
    if (!c.available()) return;
    ch = c.read();
    buff[bp++] = ch;
  } while (ch != '\n');

  buff[--bp] = 0;
  DEBUG_DUMP(buff);

  switch (state) {
    case stRequest:
      str = clean(buff);
      handleRequest(str);
      state = stHeaders;
      break;

    case stHeaders:
      str = clean(buff);
      if (strlen(str) == 0) {
        state = stData;
        page->onEndOfHeader();
        break;
      }
      page->onHeader(str);
      break;

    case stData:
      page->onData(buff, bp);
      break;
  }

  bp = 0;

  return;






  int av = c.available();
  if (!av) return;

  // Read data up to buffer length
  DEBUG_DUMP(bp);
  bp += c.readBytes(buff + bp, _min(sizeof(buff) - bp, av));
  DEBUG(Serial.print("Buffer state ["));
  DEBUG(Serial.write(buff, bp));
  DEBUG(Serial.println("]"));
  DEBUG_DUMP(bp);

  // Data passthru during data stage
  if (state == stData && page) {
    DEBUG(Serial.print("Data passthru ["));
    DEBUG(Serial.write(buff, bp));
    DEBUG(Serial.println("]"));

    // Feed data to the page
    int used = page->onData(buff, bp);

    // Save unused data for next time
    for (int i = used; i < bp; ++i) buff[i - used] = buff[i];

    return;
  }

  int sos = 0, eos = -1;
  while (1) {
    // Search for line termination
    while (eos < bp)
      if (buff[++eos] == '\n') break;

    if (eos == bp) break; // Not found

    // Line terminator found at buff[eos]
    buff[eos] = 0;

    DEBUG(Serial.print("Data received ["));
    DEBUG(Serial.print(buff + sos));
    DEBUG(Serial.println("]"));

    // Call appropriate handler
    switch (state) {
      case stRequest:
        str = clean(buff + sos);
        handleRequest(str);
        state = stHeaders;
        break;

      case stHeaders:
        str = clean(buff + sos);
        if (strlen(str) == 0) {
          state = stData;
          page->onEndOfHeader();
          break;
        }
        page->onHeader(str);
        break;
    }
  }

  // Save unused data for next round
  DEBUG(Serial.print("buffer shifting "));
  DEBUG(Serial.print(bp - eos));
  DEBUG(Serial.println(" bytes."));
  sos = 0;
  while (eos < bp) buff[sos++] = buff[eos++];
  bp = sos;

}

char *HttpServer::clean(char *str) {
  // Skip non-graphical chars at start of line
  while (*str && !isgraph(*str)) str++;

  // Find end-of-string
  char *eos = str;
  while (*eos) eos++;

  // Erase non-graphical chars at enf of line
  eos--;
  while (*eos && !isgraph(*eos)) *eos-- = 0;

  //  DEBUG(Serial.print("clean["));
  //  DEBUG(Serial.print(str));
  //  DEBUG(Serial.println("]"));

  // Return clean string
  return str;
}

void HttpServer::handleRequest(char *str) {
  // HTTP header is expected to have 3 fields
  // METHOD target?var=value PROTOCOL
  // We care about targets and query vars.
  DEBUG(Serial.print("handleRequest("));
  DEBUG(Serial.print(str));
  DEBUG(Serial.println(")"));

  // Skip over METHOD
  while (*str && *str != ' ') str++;
  if (!*str) return; // Bad header, no space found
  str++; // Skip space

  // Seek to end of target
  char *p = str;
  while (*p && *p != ' ') p++;
  if (!*p) return; // Bad header, no space found
  *p = 0; // mark as end of string

  // Search for query string
  char *query = str;
  while (*query && *query != '?') query++;
  if (*query) *query++ = 0; // Found, separate from target
  else query = 0; // No query string

  // Search for hash string
  char *hash = str;
  while (*hash && *hash != '#') hash++;
  if (*hash && (!query || hash < query)) *hash++ = 0; // Found, separate from target
  else hash = 0; // No query string

  // Check for a matching page on the list
  char *subpath = str; // Defaults to full path for use on page-not-found handler
  int pageno = -1;
  while (pages[++pageno].target) {
    // Compare up to listed target's length
    size_t n = strlen(pages[pageno].target);
    if (strncmp(pages[pageno].target, str, n) != 0) continue;

    // Exact target match
    if (str[n] == 0) {
      subpath = 0; // No subpath
      break;
    }

    // Subpath match
    if (str[n] == '/') {
      subpath = str + n + 1;
      break;
    }
  }
  page = pages[pageno].page; // Set current page
  // Please notice that 404 page must be last in the list with target=NULL

  if (pages[pageno].target) DEBUG_DUMP(pages[pageno].target);

  // Message about connection established
  page->onConnect(c, subpath, hash);

  // Iterate query strings
  while (query) {
    // Variable name is the first thing
    char *var = query;

    // Search equals sign
    char *eq = var;
    while (*eq && *eq != '=') eq++;

    // Search end of variable
    char *end = var;
    while (*end && *end != '?' && *end != '&') end++;

    // Mark query var as read
    if (*end) query = end + 1;
    else      query = 0;

    // Correct eq for cases like "var1&var2=3"
    if (eq > end) eq = 0;

    // Set eq to null pointer if no value has been set
    if (!*eq) eq = 0;

    // Separate substrings and decode
    *end  = 0;
    if (eq) {
      *eq++ = 0; // Mark end of var name

      // UrlDecode value in place
      urldecode(eq, eq);
    }

    // UrlDecode variable name in place
    urldecode(var, var);

    // Feed variable to the selected page handler
    page->onQueryVar(var, eq);
  }
}

void HttpServer::urldecode(const char *in, char *out) {
  return;
}

#endif

// This is to enforce arduino-like formatting in kate
// kate: space-indent on; indent-width 2; mixed-indent off; indent-mode cstyle;
