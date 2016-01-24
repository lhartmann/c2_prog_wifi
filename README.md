# c2_prog_wifi
WiFi-enabled programmer for Silicon Labs microcontrollers using the C2 programmer protocol, and to act as a serial-wifi bridge.

Designed to run in the Arduino environment for ESP8266 module: https://github.com/esp8266/Arduino

New programs can be loaded sending .hex files through the web-interface. You will have to edit the .ino file to set the password.

Serial bridge can be accessed through port 9600, but the baud-rate is actually 115200. Try 'nc esp8266 9600' on linux, or any other TCP/IP terminal program (maybe telnet on windows?).

Everything is still alpha. Currently tested with EFM8BB10F2G-A-QFN20 and ESP-01 module: http://www.cear.ufpb.br/~lucas.hartmann/tag/efm8bb1/

LICENSE: GPLv3 or newer.
