/** The MIT License (MIT)

Copyright (c) 2018 David Payne

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

/******************************************************************************
 * Duet RepRap Monitor is designed for the Wemos D1 ESP8266
 * Wemos D1 Mini:  https://amzn.to/2qLyKJd
 * 0.96" OLED I2C 128x64 Display (12864) SSD1306
 * OLED Display:  https://amzn.to/2JDEAUF
 ******************************************************************************/
/******************************************************************************
 * NOTE: The settings here are the default settings for the first loading.  
 * After loading you will manage changes to the settings via the Web Interface.  
 * If you want to change settings again in the settings.h, you will need to 
 * erase the file system on the Wemos or use the “Reset Settings” option in 
 * the Web Interface.
 ******************************************************************************/

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <ESP8266mDNS.h>
#include <DNSServer.h>
#include <ArduinoOTA.h>
#include "TimeClient.h"
#include "PrinterClient.h"
#include "OpenWeatherMapClient.h"
#include "WeatherStationFonts.h"
#include "FS.h"
#include "OLEDDisplayUi.h"

//******************************
// Start Settings
//******************************

// Duet RepRap Printer Monitoring -- Monitor your 3D printer 
String PrinterHostName = "Printer";
String PrinterPass = "";      // only used with haproxy or basic auth (only needed if you must authenticate)


// Weather Configuration
boolean DISPLAYWEATHER = true; // true = show weather when not printing / false = no weather
String WeatherApiKey = ""; // Your API Key from http://openweathermap.org/
// Default City Location (use http://openweathermap.org/find to find city ID)
int CityIDs[] = { 5304391 }; //Only USE ONE for weather marquee
boolean IS_METRIC = false; // false = Imperial and true = Metric

const int WEBSERVER_PORT = 80; // The port you can access this device on over HTTP
const boolean WEBSERVER_ENABLED = true;  // Device will provide a web interface via http://[ip]:[port]/
char* www_username = "admin";  // User account for the Web Interface
char* www_password = "password";  // Password for the Web Interface
float UtcOffset = -7; // Hour offset from GMT for your timezone
boolean IS_24HOUR = false;     // 23:00 millitary 24 hour clock
int minutesBetweenDataRefresh = 15;
boolean DISPLAYCLOCK = true;   // true = Show Clock when not printing / false = turn off display when not printing

// Display selection
// "All in one" board with ESP, 0.9 Oled, and LiOn 18650 style battery (there is no part number)
#include "SSD1306Wire.h"
const int I2C_DISPLAY_ADDRESS = 0x3c; 
const int SDA_PIN = D3;
const int SCL_PIN = D4;
const boolean INVERT_DISPLAY = true; // true = pins at top | false = pins at the bottom

// Wemos Mini D1 with display plugged in so it gets +5 and GND and next two pins.
//#include "SSD1306Wire.h"
//const int I2C_DISPLAY_ADDRESS = 0x78; 
//const int SDA_PIN = D3;
//const int SCL_PIN = D4;
//const boolean INVERT_DISPLAY = true; // true = pins at top | false = pins at the bottom

// Wemos Mini D1 with display plugged in so it gets +5 and GND and next two pins.
//#include "SH1106Wire.h"
//#define DISPLAY_SH1106       // Uncomment this line to use the SH1106 display -- SSD1306 is used by default and is most common
//const int I2C_DISPLAY_ADDRESS = 0x78;
//const int SDA_PIN = D3;
//const int SCL_PIN = D4;
//const boolean INVERT_DISPLAY = false; // true = pins at top | false = pins at the bottom
//#define DISPLAY_SH1106       // Uncomment this line to use the SH1106 display -- SSD1306 is used by default and is most common
// End of display selection


boolean ENABLE_OTA = true;     // this will allow you to load firmware to the device over WiFi (see OTA for ESP8266)
String OTA_Password = "";      // Set an OTA password here -- leave blank if you don't want to be prompted for password
//******************************
// End Settings
//******************************

String themeColor = "light-green"; // this can be changed later in the web interface.
