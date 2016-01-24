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

#include <Arduino.h>
#include "httpServerPage.h"

#define DEBUG(x) (x)

void HttpServerPage::onConnect(WiFiClient &client, const char *sub, const char *hash) {
  DEBUG(Serial.print("onConnect(client,\""));
  DEBUG(Serial.print(sub));
  DEBUG(Serial.print("\",\""));
  DEBUG(Serial.print(hash));
  DEBUG(Serial.println("\")"));
  c = &client;
}

void HttpServerPage::onQueryVar(char *var, char *value) {
  DEBUG(Serial.print("onQueryVar("));
  DEBUG(Serial.print(var));
  DEBUG(Serial.print(","));
  DEBUG(Serial.print(value));
  DEBUG(Serial.println(")"));
}

void HttpServerPage::onHeader(char *str) {
  DEBUG(Serial.print("onHeader("));
  DEBUG(Serial.print(str));
  DEBUG(Serial.println(")"));
}

void HttpServerPage::onEndOfHeader() {
  DEBUG(Serial.println("onEndOfHeader(...)"));
  c->stop();
}

int HttpServerPage::onData(char *str, int size) {
  DEBUG(Serial.print("onData(\""));
  DEBUG(Serial.write(str,size));
  DEBUG(Serial.print("\","));
  DEBUG(Serial.print(size));
  DEBUG(Serial.println(")"));
  return size;
}

void HttpServerPage::onClose() {
  DEBUG(Serial.println("onClose()"));
}

// This is to enforce arduino-like formatting in kate
// kate: space-indent on; indent-width 2; mixed-indent off; indent-mode cstyle;
