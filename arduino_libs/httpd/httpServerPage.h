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

#ifndef HTTPSERVERPAGE_H
#define HTTPSERVERPAGE_H

#include <WiFiClient.h>

// Http Server metapage

struct HttpServerPage {
  WiFiClient *c;
  virtual void onConnect(WiFiClient &client, const char *sub, const char *hash);
  virtual void onQueryVar(char *var, char *value);
  virtual void onHeader(char *str);
  virtual void onEndOfHeader();
  virtual int onData(char *str, int size);
  virtual void onClose();
};

#endif // HTTPSERVERPAGE_H

// This is to enforce arduino-like formatting in kate
// kate: space-indent on; indent-width 2; mixed-indent off; indent-mode cstyle;
