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
 
 /**********************************************
 * Edit Settings.h for personalization
 ***********************************************/

#include "Settings.h"

#define VERSION "1.0"

#define HOSTNAME "DuetMon-" 
#define CONFIG "/conf.txt"

/* Useful Constants */
#define SECS_PER_MIN  (60UL)
#define SECS_PER_HOUR (3600UL)
#define SECS_PER_DAY  (SECS_PER_HOUR * 24L)

/* Useful Macros for getting elapsed time */
#define numberOfSeconds(_time_) (_time_ % SECS_PER_MIN)  
#define numberOfMinutes(_time_) ((_time_ / SECS_PER_MIN) % SECS_PER_MIN) 
#define numberOfHours(_time_) (( _time_% SECS_PER_DAY) / SECS_PER_HOUR)
#define elapsedDays(_time_) ( _time_ / SECS_PER_DAY)  

// Initialize the oled display for I2C_DISPLAY_ADDRESS
// SDA_PIN and SCL_PIN
#if defined(DISPLAY_SH1106)
  SH1106Wire     display(I2C_DISPLAY_ADDRESS, SDA_PIN, SCL_PIN);
#else
  SSD1306Wire     display(I2C_DISPLAY_ADDRESS, SDA_PIN, SCL_PIN); // this is the default
#endif

OLEDDisplayUi   ui( &display );

void drawProgress(OLEDDisplay *display, int percentage, String label);
void drawOtaProgress(unsigned int, unsigned int);
void drawScreen1(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void drawScreen2(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void drawScreen3(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void drawHeaderOverlay(OLEDDisplay *display, OLEDDisplayUiState* state);
void drawClock(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void drawWeather(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void drawClockHeaderOverlay(OLEDDisplay *display, OLEDDisplayUiState* state);

// Set the number of Frames supported
const int numberOfFrames = 3;
FrameCallback frames[numberOfFrames];
FrameCallback clockFrame[2];
boolean isClockOn = false;

OverlayCallback overlays[] = { drawHeaderOverlay };
OverlayCallback clockOverlay[] = { drawClockHeaderOverlay };
int numberOfOverlays = 1;

// Time 
TimeClient timeClient(UtcOffset);
long lastEpoch = 0;
long firstEpoch = 0;
long displayOffEpoch = 0;
String lastMinute = "xx";
String lastSecond = "xx";
String lastReportStatus = "";
boolean displayOn = true;

// DuetRepRap Client
PrinterClient PrinterClient(PrinterHostName, PrinterPass);
int printerCount = 0;

// Weather Client
OpenWeatherMapClient weatherClient(WeatherApiKey, CityIDs, 1, IS_METRIC);

//declairing prototypes
void configModeCallback (WiFiManager *myWiFiManager);
int8_t getWifiQuality();

ESP8266WebServer server(WEBSERVER_PORT);
DNSServer dnsServer;
const byte DNS_PORT = 53;

String WEB_ACTIONS =  "<a class='w3-bar-item w3-button' href='/'><i class='fa fa-home'></i> Home</a>"
                      "<a class='w3-bar-item w3-button' href='/configure'><i class='fa fa-cog'></i> Configure</a>"
                      "<a class='w3-bar-item w3-button' href='/configureweather'><i class='fa fa-cloud'></i> Weather</a>"
                      "<a class='w3-bar-item w3-button' href='/systemreset' onclick='return confirm(\"Do you want to reset to default settings?\")'><i class='fa fa-undo'></i> Reset Settings</a>"
                      "<a class='w3-bar-item w3-button' href='/forgetwifi' onclick='return confirm(\"Do you want to forget to WiFi connection?\")'><i class='fa fa-wifi'></i> Forget WiFi</a>"
                      "<a class='w3-bar-item w3-button' href='https://github.com/Qrome' target='_blank'><i class='fa fa-question-circle'></i> About</a>";
                            
String CHANGE_FORM =  "<form class='w3-container' action='/updateconfig' method='get'><h2>Station Config:</h2>"
                      "<p><label>Printer Host Name (or IP)</label><input class='w3-input w3-border w3-margin-bottom' type='text' name='PrinterHostName' value='%PrinterHostName%' maxlength='60'></p>"
                      "<p><label>Printer Password (or leave blank)</label><input class='w3-input w3-border w3-margin-bottom' type='password' name='printerPass' value='%printerPass%'></p><hr>"
                      "<p><input name='isClockEnabled' class='w3-check w3-margin-top' type='checkbox' %IS_CLOCK_CHECKED%> Display Clock when printer is off</p>"
                      "<p><input name='is24hour' class='w3-check w3-margin-top' type='checkbox' %IS_24HOUR_CHECKED%> Use 24 Hour Clock (military time)</p>"
                      "<p>Clock Sync / Weather Refresh (minutes) <select class='w3-option w3-padding' name='refresh'>%OPTIONS%</select></p>"
                      "<p>Theme Color <select class='w3-option w3-padding' name='theme'>%THEME_OPTIONS%</select></p>"
                      "<p><label>UTC Time Offset</label><input class='w3-input w3-border w3-margin-bottom' type='text' name='utcoffset' value='%UTCOFFSET%' maxlength='12'></p><hr>"
                      "<p><label>User ID (for this interface)</label><input class='w3-input w3-border w3-margin-bottom' type='text' name='userid' value='%USERID%' maxlength='20'></p>"
                      "<p><label>Password </label><input class='w3-input w3-border w3-margin-bottom' type='password' name='stationpassword' value='%STATIONPASSWORD%'></p>"
                      "<button class='w3-button w3-block w3-grey w3-section w3-padding' type='submit'>Save</button></form>";

String WEATHER_FORM = "<form class='w3-container' action='/updateweatherconfig' method='get'><h2>Weather Config:</h2>"
                      "<p><input name='isWeatherEnabled' class='w3-check w3-margin-top' type='checkbox' %IS_WEATHER_CHECKED%> Display Weather when printer is off</p>"
                      "<label>OpenWeatherMap API Key (get from <a href='https://openweathermap.org/' target='_BLANK'>here</a>)</label>"
                      "<input class='w3-input w3-border w3-margin-bottom' type='text' name='openWeatherMapApiKey' value='%WEATHERKEY%' maxlength='60'>"
                      "<p><label>%CITYNAME1% (<a href='http://openweathermap.org/find' target='_BLANK'><i class='fa fa-search'></i> Search for City ID</a>) "
                      "or full <a href='http://openweathermap.org/help/city_list.txt' target='_BLANK'>city list</a></label>"
                      "<input class='w3-input w3-border w3-margin-bottom' type='text' name='city1' value='%CITY1%' onkeypress='return isNumberKey(event)'></p>"
                      "<p><input name='metric' class='w3-check w3-margin-top' type='checkbox' %METRIC%> Use Metric (Celsius)</p>"
                      "<button class='w3-button w3-block w3-grey w3-section w3-padding' type='submit'>Save</button></form>"
                      "<script>function isNumberKey(e){var h=e.which?e.which:event.keyCode;return!(h>31&&(h<48||h>57))}</script>";

String COLOR_THEMES = "<option>red</option>"
                      "<option>pink</option>"
                      "<option>purple</option>"
                      "<option>deep-purple</option>"
                      "<option>indigo</option>"
                      "<option>blue</option>"
                      "<option>light-blue</option>"
                      "<option>cyan</option>"
                      "<option>teal</option>"
                      "<option>green</option>"
                      "<option>light-green</option>"
                      "<option>lime</option>"
                      "<option>khaki</option>"
                      "<option>yellow</option>"
                      "<option>amber</option>"
                      "<option>orange</option>"
                      "<option>deep-orange</option>"
                      "<option>blue-grey</option>"
                      "<option>brown</option>"
                      "<option>grey</option>"
                      "<option>dark-grey</option>"
                      "<option>black</option>"
                      "<option>w3schools</option>";
                            

// Change the externalLight to the pin you wish to use if other than the Built-in LED
int externalLight = LED_BUILTIN; // LED_BUILTIN is is the built in LED on the Wemos

void setup() {
  Serial.begin(115200);
  //New Line to clear from start garbage
  Serial.println();
  delay(10);
  Serial.println("DuetMonitor starting.");  
  
  SPIFFS.begin();
  delay(10);
  
  // Initialize digital pin for LED (little blue light on the Wemos D1 Mini)
  pinMode(externalLight, OUTPUT);

  readSettings();

  // initialize dispaly
  display.init();
  if (INVERT_DISPLAY) {
    display.flipScreenVertically(); // connections at top of OLED display
  }
  display.clear();
  display.display();

  //display.flipScreenVertically();
  display.setFont(ArialMT_Plain_16);
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.setContrast(255); // default is 255
  display.drawString(64, 5, "Duet3D Monitor\nBy Danal\nV" + String(VERSION));
  display.display();

  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;
  
  // Uncomment for testing wifi manager
  //wifiManager.resetSettings();
  wifiManager.setAPCallback(configModeCallback);
  
  String hostname(HOSTNAME);
  hostname += String(ESP.getChipId(), HEX);
  if (!wifiManager.autoConnect((const char *)hostname.c_str())) {// new addition
    delay(3000);
    WiFi.disconnect(true);
    ESP.reset();
    delay(5000);
  }
  
  // You can change the transition that is used
  // SLIDE_LEFT, SLIDE_RIGHT, SLIDE_TOP, SLIDE_DOWN
  ui.setFrameAnimation(SLIDE_LEFT);
  ui.setTargetFPS(10);
  ui.disableAllIndicators();
  ui.setFrames(frames, (numberOfFrames));
  frames[0] = drawScreen1;
  frames[1] = drawScreen2;
  frames[2] = drawScreen3;
  clockFrame[0] = drawClock;
  clockFrame[1] = drawWeather;
  ui.setOverlays(overlays, numberOfOverlays);
  
  // Inital UI takes care of initalising the display too.
  ui.init();
  if (INVERT_DISPLAY) {
    display.flipScreenVertically();  //connections at top of OLED display
  }
  
  // print the received signal strength:
  Serial.print("Signal Strength (RSSI): ");
  Serial.print(getWifiQuality());
  Serial.println("%");

  // print the received signal strength:
  Serial.print("Sleep Type: ");
  Serial.print(getWifiSleepType());
  Serial.println("");

  if (ENABLE_OTA) {
    ArduinoOTA.onStart([]() {
      Serial.println("Start");
    });
    ArduinoOTA.onEnd([]() {
      Serial.println("\nEnd");
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    });
    ArduinoOTA.onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });
    ArduinoOTA.setHostname((const char *)hostname.c_str()); 
    if (OTA_Password != "") {
      ArduinoOTA.setPassword(((const char *)OTA_Password.c_str()));
    }
    ArduinoOTA.begin();
  }

  if (WEBSERVER_ENABLED) {
    server.on("/", displayPrinterStatus);
    server.on("/systemreset", handleSystemReset);
    server.on("/forgetwifi", handleWifiReset);
    server.on("/updateconfig", handleUpdateConfig);
    server.on("/updateweatherconfig", handleUpdateWeather);
    server.on("/configure", handleConfigure);
    server.on("/configureweather", handleWeatherConfigure);
    server.onNotFound(redirectHome);

    // Start a DNS server that acts like a captive portal. 
    dnsServer.start(DNS_PORT, "*", WiFi.localIP());
    Serial.println("DNS server started, all roads lead here");
    // Start the server
    server.begin();
    Serial.println("Web Server started");
    // Print the IP address
    String webAddress = "http://" + WiFi.localIP().toString() + ":" + String(WEBSERVER_PORT) + "/";
    Serial.println("Use this URL : " + webAddress);
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.setFont(ArialMT_Plain_10);
    display.drawString(64, 10, "Web Interface On");
    display.drawString(64, 20, "You May Connect to IP");
    display.setFont(ArialMT_Plain_16);
    display.drawString(64, 30, WiFi.localIP().toString());
    display.drawString(64, 46, "Port: " + String(WEBSERVER_PORT));
    display.display();
  } else {
    Serial.println("Web Interface is Disabled");
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.setFont(ArialMT_Plain_10);
    display.drawString(64, 10, "Web Interface is Off");
    display.drawString(64, 20, "Enable in Settings.h");
    display.display(); 
  }
   
  flashLED(5, 500);
  findMDNS();  //go find Print Server by the hostname
}

void findMDNS() {
  if (PrinterHostName == "") {
    return; // nothing to do here
  }
  // We now query our network for 'web servers' service
  // over tcp, and get the number of available devices
  int n = MDNS.queryService("http", "tcp");
  if (n == 0) {
    Serial.println("no services found - make sure Printer is turned on");
    return;
  }
  Serial.println("*** Looking for " + PrinterHostName + " over mDNS");
  for (int i = 0; i < n; ++i) {
    // Going through every available service,
    // we're searching for the one whose hostname
    // matches what we want, and then get its IP
    Serial.println("Found: " + MDNS.hostname(i));
    if (MDNS.hostname(i) == PrinterHostName) {
      IPAddress serverIp = MDNS.IP(i);
      PrinterHostName = serverIp.toString();
      Serial.println("*** Found Printer Server " + PrinterHostName + " http://" + PrinterHostName);
      writeSettings(); // update the settings
    }
  }
}

//************************************************************
// Main Loop
//************************************************************
void loop() {
  
   //Get Time Update
  if((getMinutesFromLastRefresh() >= minutesBetweenDataRefresh) || lastEpoch == 0) {
    getUpdateTime();
  }

  if (lastMinute != timeClient.getMinutes() && !PrinterClient.isPrinting()) {
    // Check status every 60 seconds
    digitalWrite(externalLight, LOW);
    lastMinute = timeClient.getMinutes(); // reset the check value
    PrinterClient.getPrinterJobResults();
    digitalWrite(externalLight, HIGH);
  } else if (PrinterClient.isPrinting()) {
    if (lastSecond != timeClient.getSeconds() && timeClient.getSeconds().endsWith("0")) {
      lastSecond = timeClient.getSeconds();
      // every 10 seconds while printing get an update
      digitalWrite(externalLight, LOW);
      PrinterClient.getPrinterJobResults();
      digitalWrite(externalLight, HIGH);
    }
  }

  checkDisplay(); // Check to see if the printer is on or offline and change display.
  ui.update();

  if (WEBSERVER_ENABLED) {
    server.handleClient();
    dnsServer.processNextRequest();
  }
  if (ENABLE_OTA) {
    ArduinoOTA.handle();
  }
}

// End of Main Loop

void getUpdateTime() {
  digitalWrite(externalLight, LOW); // turn on the LED
  Serial.println();

  if (displayOn && DISPLAYWEATHER) {
    Serial.println("Getting Weather Data...");
    weatherClient.updateWeather();
  }

  Serial.println("Updating Time...");
  //Update the Time
  timeClient.updateTime();
  lastEpoch = timeClient.getCurrentEpoch();
  Serial.println("Local time: " + timeClient.getAmPmFormattedTime());

  digitalWrite(externalLight, HIGH);  // turn off the LED
}

void handleSystemReset() {
  if (!server.authenticate(www_username, www_password)) {
    return server.requestAuthentication();
  }
  Serial.println("Reset System Configuration");
  if (SPIFFS.remove(CONFIG)) {
    redirectHome();
    ESP.restart();
  }
}

void handleUpdateWeather() {
  if (!server.authenticate(www_username, www_password)) {
    return server.requestAuthentication();
  }
  DISPLAYWEATHER = server.hasArg("isWeatherEnabled");
  WeatherApiKey = server.arg("openWeatherMapApiKey");
  CityIDs[0] = server.arg("city1").toInt();
  IS_METRIC = server.hasArg("metric");
  writeSettings();
  isClockOn = false; // this will force a check for the display
  checkDisplay();
  lastEpoch = 0;
  redirectHome();
}

void handleUpdateConfig() {
  if (!server.authenticate(www_username, www_password)) {
    return server.requestAuthentication();
  }
  PrinterHostName = server.arg("PrinterHostName");
  PrinterPass = server.arg("printerPass");
  DISPLAYCLOCK = server.hasArg("isClockEnabled");
  IS_24HOUR = server.hasArg("is24hour");
  minutesBetweenDataRefresh = server.arg("refresh").toInt();
  themeColor = server.arg("theme");
  UtcOffset = server.arg("utcoffset").toFloat();
  String temp = server.arg("userid");
  temp.toCharArray(www_username, sizeof(temp));
  temp = server.arg("stationpassword");
  temp.toCharArray(www_password, sizeof(temp));
  writeSettings();
  findMDNS();
  PrinterClient.getPrinterJobResults();
  checkDisplay();
  lastEpoch = 0;
  redirectHome();
}

void handleWifiReset() {
  if (!server.authenticate(www_username, www_password)) {
    return server.requestAuthentication();
  }
  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  redirectHome();
  WiFiManager wifiManager;
  wifiManager.resetSettings();
  ESP.restart();
}

void handleWeatherConfigure() {
  if (!server.authenticate(www_username, www_password)) {
    return server.requestAuthentication();
  }
  digitalWrite(externalLight, LOW);
  String html = "";

  server.sendHeader("Cache-Control", "no-cache, no-store");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", "");

  html = getHeader();
  server.sendContent(html);
  
  String form = WEATHER_FORM;
  String isWeatherChecked = "";
  if (DISPLAYWEATHER) {
    isWeatherChecked = "checked='checked'";
  }
  form.replace("%IS_WEATHER_CHECKED%", isWeatherChecked);
  form.replace("%WEATHERKEY%", WeatherApiKey);
  form.replace("%CITYNAME1%", weatherClient.getCity(0));
  form.replace("%CITY1%", String(CityIDs[0]));
  String checked = "";
  if (IS_METRIC) {
    checked = "checked='checked'";
  }
  form.replace("%METRIC%", checked);

  server.sendContent(form);
  
  html = getFooter();
  server.sendContent(html);
  server.sendContent("");
  server.client().stop();
  digitalWrite(externalLight, HIGH);
}

void handleConfigure() {
  if (!server.authenticate(www_username, www_password)) {
    return server.requestAuthentication();
  }
  digitalWrite(externalLight, LOW);
  String html = "";

  server.sendHeader("Cache-Control", "no-cache, no-store");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", "");

  html = getHeader();
  server.sendContent(html);
  
  String form = CHANGE_FORM;
  
  form.replace("%PrinterHostName%", PrinterHostName);
  form.replace("%printerPass%", PrinterPass);
  String isClockChecked = "";
  if (DISPLAYCLOCK) {
    isClockChecked = "checked='checked'";
  }
  form.replace("%IS_CLOCK_CHECKED%", isClockChecked);
  String is24hourChecked = "";
  if (IS_24HOUR) {
    is24hourChecked = "checked='checked'";
  }
  form.replace("%IS_24HOUR_CHECKED%", is24hourChecked);
  String options = "<option>10</option><option>15</option><option>20</option><option>30</option><option>60</option>";
  options.replace(">"+String(minutesBetweenDataRefresh)+"<", " selected>"+String(minutesBetweenDataRefresh)+"<");
  form.replace("%OPTIONS%", options);
  String themeOptions = COLOR_THEMES;
  themeOptions.replace(">"+String(themeColor)+"<", " selected>"+String(themeColor)+"<");
  form.replace("%THEME_OPTIONS%", themeOptions);
  form.replace("%UTCOFFSET%", String(UtcOffset));
  form.replace("%USERID%", String(www_username));
  form.replace("%STATIONPASSWORD%", String(www_password));

  server.sendContent(form);
  
  html = getFooter();
  server.sendContent(html);
  server.sendContent("");
  server.client().stop();
  digitalWrite(externalLight, HIGH);
}

/*
void displayMessage(String message) {
  digitalWrite(externalLight, LOW);

  server.sendHeader("Cache-Control", "no-cache, no-store");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", "");
  String html = getHeader();
  server.sendContent(String(html));
  server.sendContent(String(message));
  html = getFooter();
  server.sendContent(String(html));
  server.sendContent("");
  server.client().stop();
  
  digitalWrite(externalLight, HIGH);
}
*/
void redirectHome() {
  // Send them back to the Root Directory
  server.sendHeader("Location", String("/"), true);
  server.sendHeader("Cache-Control", "no-cache, no-store");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.send(302, "text/plain", "");
  server.client().stop();
}

String getHeader() {
  return getHeader(false);
}

String getHeader(boolean refresh) {
  String menu = WEB_ACTIONS;

  String html = "<!DOCTYPE HTML>";
  html += "<html><head><title>Duet Monitor</title><link rel='icon' href='data:;base64,='>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  if (refresh) {
    html += "<meta http-equiv=\"refresh\" content=\"30\">";
  }
  html += "<link rel='stylesheet' href='https://www.w3schools.com/w3css/4/w3.css'>";
  html += "<link rel='stylesheet' href='https://www.w3schools.com/lib/w3-theme-" + themeColor + ".css'>";
  html += "<link rel='stylesheet' href='https://cdnjs.cloudflare.com/ajax/libs/font-awesome/4.7.0/css/font-awesome.min.css'>";
  html += "</head><body>";
  html += "<nav class='w3-sidebar w3-bar-block w3-card' style='margin-top:88px' id='mySidebar'>";
  html += "<div class='w3-container w3-theme-d2'>";
  html += "<span onclick='closeSidebar()' class='w3-button w3-display-topright w3-large'><i class='fa fa-times'></i></span>";
  html += "<div class='w3-cell w3-left w3-xxxlarge' style='width:60px'><i class='fa fa-cube'></i></div>";
  html += "<div class='w3-padding'>Menu</div></div>";
  html += menu;
  html += "</nav>";
  html += "<header class='w3-top w3-bar w3-theme'><button class='w3-bar-item w3-button w3-xxxlarge w3-hover-theme' onclick='openSidebar()'><i class='fa fa-bars'></i></button><h2 class='w3-bar-item'>Duet Monitor</h2></header>";
  html += "<script>";
  html += "function openSidebar(){document.getElementById('mySidebar').style.display='block'}function closeSidebar(){document.getElementById('mySidebar').style.display='none'}closeSidebar();";
  html += "</script>";
  html += "<br><div class='w3-container w3-large' style='margin-top:88px'>";
  return html;
}

String getFooter() {
  int8_t rssi = getWifiQuality();
  Serial.print("Signal Strength (RSSI): ");
  Serial.print(rssi);
  Serial.println("%");
  String html = "<br><br><br>";
  html += "</div>";
  html += "<footer class='w3-container w3-bottom w3-theme w3-margin-top'>";
  if (lastReportStatus != "") {
    html += "<i class='fa fa-external-link'></i> Report Status: " + lastReportStatus + "<br>";
  }
  html += "<i class='fa fa-paper-plane-o'></i> Version: " + String(VERSION) + "<br>";
  html += "<i class='fa fa-rss'></i> Signal Strength: ";
  html += String(rssi) + "%";
  html += "</footer>";
  html += "</body></html>";
  return html;
}

void displayPrinterStatus() {
  digitalWrite(externalLight, LOW);
  String html = "";

  server.sendHeader("Cache-Control", "no-cache, no-store");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", "");
  server.sendContent(String(getHeader(true)));

  String displayTime = timeClient.getAmPmHours() + ":" + timeClient.getMinutes() + ":" + timeClient.getSeconds() + " " + timeClient.getAmPm();
  if (IS_24HOUR) {
    displayTime = timeClient.getHours() + ":" + timeClient.getMinutes() + ":" + timeClient.getSeconds();
  }
  
  html += "<div class='w3-cell-row' style='width:100%'><h2>Time: " + displayTime + "</h2></div><div class='w3-cell-row'>";
  html += "<div class='w3-cell w3-container' style='width:100%'><p>";
  html += "Host Name: " + PrinterHostName + "<br>";
  if (PrinterClient.getError() != "") {
    html += "Error: " + PrinterClient.getError() + "<br>";
  }
  html += "Status: " + PrinterClient.getStatus() + "<br>";

  if (PrinterClient.isPrinting()) {
    html += "File: " + PrinterClient.getFileName() + "<br>";
    float fileSize = PrinterClient.getFileSize().toFloat();
    if (fileSize > 0) {
      fileSize = fileSize / 1024;
      html += "File Size: " + String(fileSize) + "KB<br>";
    }
    int filamentLength = PrinterClient.getFilamentLength().toInt();
    if (filamentLength > 0) {
      float fLength = float(filamentLength) / 1000;
      html += "Filament: " + String(fLength) + "m<br>";
    }
  
    html += "Tool Temperature: " + PrinterClient.getTempToolActual() + "&#176; C<br>";
    html += "Bed Temperature: " + PrinterClient.getTempBedActual() + "&#176; C<br>";
    
    int val = PrinterClient.getProgressPrintTimeLeft().toInt();
    int days = elapsedDays(val);
    int hours = numberOfHours(val);
    int minutes = numberOfMinutes(val);
    int seconds = numberOfSeconds(val);
    html += "Est. Print Time Left: " + zeroPad(hours) + ":" + zeroPad(minutes) + ":" + zeroPad(seconds) + "<br>";
  
    val = PrinterClient.getProgressPrintTime().toInt();
    days = elapsedDays(val);
    hours = numberOfHours(val);
    minutes = numberOfMinutes(val);
    seconds = numberOfSeconds(val);
    html += "Printing Time: " + zeroPad(hours) + ":" + zeroPad(minutes) + ":" + zeroPad(seconds) + "<br>";
    html += "<style>#myProgress {width: 100%;background-color: #ddd;}#myBar {width: " + PrinterClient.getProgressCompletion() + "%;height: 30px;background-color: #4CAF50;}</style>";
    html += "<div id=\"myProgress\"><div id=\"myBar\" class=\"w3-medium w3-center\">" + PrinterClient.getProgressCompletion() + "%</div></div>";
  } else {
    html += "<hr>";
  }

  html += "</p></div></div>";

  server.sendContent(html); // spit out what we got
  html = "";

  if (DISPLAYWEATHER) {
    if (weatherClient.getCity(0) == "") {
      html += "<p>Please <a href='/configureweather'>Configure Weather</a> API</p>";
      if (weatherClient.getError() != "") {
        html += "<p>Weather Error: <strong>" + weatherClient.getError() + "</strong></p>";
      }
    } else {
      html += "<div class='w3-cell-row' style='width:100%'><h2>" + weatherClient.getCity(0) + ", " + weatherClient.getCountry(0) + "</h2></div><div class='w3-cell-row'>";
      html += "<div class='w3-cell w3-left w3-medium' style='width:120px'>";
      html += "<img src='http://openweathermap.org/img/w/" + weatherClient.getIcon(0) + ".png' alt='" + weatherClient.getDescription(0) + "'><br>";
      html += weatherClient.getHumidity(0) + "% Humidity<br>";
      html += weatherClient.getWind(0) + " <span class='w3-tiny'>" + getSpeedSymbol() + "</span> Wind<br>";
      html += "</div>";
      html += "<div class='w3-cell w3-container' style='width:100%'><p>";
      html += weatherClient.getCondition(0) + " (" + weatherClient.getDescription(0) + ")<br>";
      html += weatherClient.getTempRounded(0) + getTempSymbol(true) + "<br>";
      html += "<a href='https://www.google.com/maps/@" + weatherClient.getLat(0) + "," + weatherClient.getLon(0) + ",10000m/data=!3m1!1e3' target='_BLANK'><i class='fa fa-map-marker' style='color:red'></i> Map It!</a><br>";
      html += "</p></div></div>";
    }
    
    server.sendContent(html); // spit out what we got
    html = ""; // fresh start
  }

  server.sendContent(String(getFooter()));
  server.sendContent("");
  server.client().stop();
  digitalWrite(externalLight, HIGH);
}

void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("***********************");
  Serial.println("* Entered config mode *");
  Serial.println("***********************");
  Serial.println(WiFi.softAPIP());

  display.clear();
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.setFont(ArialMT_Plain_10);
  display.drawString(64, 0, "Wifi Manager");
  display.drawString(64, 10, "Please connect to AP");
  display.setFont(ArialMT_Plain_16);
  display.drawString(64, 23, myWiFiManager->getConfigPortalSSID());
  display.setFont(ArialMT_Plain_10);
  display.drawString(64, 42, "To setup Wifi connection");
  display.display();
  
  Serial.println("Wifi Manager");
  Serial.println("Please connect to AP");
  Serial.println(myWiFiManager->getConfigPortalSSID());
  Serial.println("To setup Wifi Configuration");
  flashLED(20, 50);
}

void flashLED(int number, int delayTime) {
  for (int inx = 0; inx < number; inx++) {
      delay(delayTime);
      digitalWrite(externalLight, LOW);
      delay(delayTime);
      digitalWrite(externalLight, HIGH);
      delay(delayTime);
  }
}

void drawScreen1(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_16);
  display->drawString(64 + x, 0 + y, "Bed / Tool Temp");
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(ArialMT_Plain_24);
  String bed = PrinterClient.getValueRounded(PrinterClient.getTempBedActual());
  String tool = PrinterClient.getValueRounded(PrinterClient.getTempToolActual());
  display->drawString(2 + x, 14 + y, bed + "°");
  display->drawString(64 + x, 14 + y, tool + "°");
}

void drawScreen2(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_16);

  display->drawString(64 + x, 0 + y, "Time Remaining");
  //display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(ArialMT_Plain_24);
  int val = PrinterClient.getProgressPrintTimeLeft().toInt();
  int days = elapsedDays(val);
  int hours = numberOfHours(val);
  int minutes = numberOfMinutes(val);
  int seconds = numberOfSeconds(val);

  String time = zeroPad(hours) + ":" + zeroPad(minutes) + ":" + zeroPad(seconds);
  display->drawString(64 + x, 14 + y, time);
}

void drawScreen3(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_16);

  display->drawString(64 + x, 0 + y, "Printing Time");
  //display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(ArialMT_Plain_24);
  int val = PrinterClient.getProgressPrintTime().toInt();
  int days = elapsedDays(val);
  int hours = numberOfHours(val);
  int minutes = numberOfMinutes(val);
  int seconds = numberOfSeconds(val);

  String time = zeroPad(hours) + ":" + zeroPad(minutes) + ":" + zeroPad(seconds);
  display->drawString(64 + x, 14 + y, time);
}

void drawClock(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  
  String displayTime = timeClient.getAmPmHours() + ":" + timeClient.getMinutes() + ":" + timeClient.getSeconds();
  if (IS_24HOUR) {
    displayTime = timeClient.getHours() + ":" + timeClient.getMinutes() + ":" + timeClient.getSeconds(); 
  }
  display->setFont(ArialMT_Plain_16);
  display->drawString(64 + x, 0 + y, PrinterHostName);
  display->setFont(ArialMT_Plain_24);
  display->drawString(64 + x, 17 + y, displayTime);
}

void drawWeather(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(ArialMT_Plain_24);
  display->drawString(0 + x, 0 + y, weatherClient.getTempRounded(0) + getTempSymbol());
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(ArialMT_Plain_24);

  display->setFont(ArialMT_Plain_16);
  display->drawString(0 + x, 24 + y, weatherClient.getCondition(0));
  display->setFont((const uint8_t*)Meteocons_Plain_42);
  display->drawString(86 + x, 0 + y, weatherClient.getWeatherIcon(0));
}

String getTempSymbol() {
  return getTempSymbol(false);
}

String getTempSymbol(boolean forHTML) {
  String rtnValue = "F";
  if (IS_METRIC) {
    rtnValue = "C";
  }
  if (forHTML) {
    rtnValue = "&#176;" + rtnValue;
  } else {
    rtnValue = "°" + rtnValue;
  }
  return rtnValue;
}

String getSpeedSymbol() {
  String rtnValue = "mph";
  if (IS_METRIC) {
    rtnValue = "kph";
  }
  return rtnValue;
}

String zeroPad(int value) {
  String rtnValue = String(value);
  if (value < 10) {
    rtnValue = "0" + rtnValue;
  }
  return rtnValue;
}

void drawHeaderOverlay(OLEDDisplay *display, OLEDDisplayUiState* state) {
  display->setColor(WHITE);
  display->setFont(ArialMT_Plain_16);
  String displayTime = timeClient.getAmPmHours() + ":" + timeClient.getMinutes();
  if (IS_24HOUR) {
    displayTime = timeClient.getHours() + ":" + timeClient.getMinutes();
  }
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->drawString(0, 48, displayTime);
  
  if (!IS_24HOUR) {
    String ampm = timeClient.getAmPm();
    display->setFont(ArialMT_Plain_10);
    display->drawString(39, 54, ampm);
  }

  display->setFont(ArialMT_Plain_16);
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  String percent = String(PrinterClient.getProgressCompletion()) + "%";
  display->drawString(64, 48, percent);
  
  // Draw indicator to show next update
  int updatePos = (PrinterClient.getProgressCompletion().toFloat() / float(100)) * 128;
  display->drawRect(0, 41, 128, 6);
  display->drawHorizontalLine(0, 42, updatePos);
  display->drawHorizontalLine(0, 43, updatePos);
  display->drawHorizontalLine(0, 44, updatePos);
  display->drawHorizontalLine(0, 45, updatePos);
  
  drawRssi(display);
}

void drawClockHeaderOverlay(OLEDDisplay *display, OLEDDisplayUiState* state) {
  display->setColor(WHITE);
  display->setFont(ArialMT_Plain_16);
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  if (!IS_24HOUR) {
    display->drawString(0, 48, timeClient.getAmPm());
    display->setTextAlignment(TEXT_ALIGN_CENTER);
    display->drawString(64, 48, "Printer Idle");
  } else {
    display->drawString(0,48, "Printer Idle");
  }
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->drawRect(0, 43, 128, 2);
 
  drawRssi(display);
}

void drawRssi(OLEDDisplay *display) {
  int8_t quality = getWifiQuality();
  for (int8_t i = 0; i < 4; i++) {
    for (int8_t j = 0; j < 3 * (i + 2); j++) {
      if (quality > i * 25 || j == 0) {
        display->setPixel(114 + 4 * i, 63 - j);
      }
    }
  }
}

// converts the dBm to a range between 0 and 100%
int8_t getWifiQuality() {
  int32_t dbm = WiFi.RSSI();
  if(dbm <= -100) {
      return 0;
  } else if(dbm >= -50) {
      return 100;
  } else {
      return 2 * (dbm + 100);
  }
}

// converts the dBm to a range between 0 and 100%
String getWifiSleepType() {
  WiFiSleepType_t tmp = WiFi.getSleepMode();
  if(tmp == 0) {
      return "No Sleep";
  } else if(tmp == 1) {
      return "Light Sleep";
  } else if(tmp == 2) {
      return "Modem Sleep";
  } else {
      return "Sleep Type Unknown";
  }
}

void writeSettings() {
  // Save decoded message to SPIFFS file for playback on power up.
  File f = SPIFFS.open(CONFIG, "w");
  if (!f) {
    Serial.println("File open failed!");
  } else {
    Serial.println("Saving settings now...");
    f.println("UtcOffset=" + String(UtcOffset));
    f.println("PrinterHostName=" + PrinterHostName);
    f.println("printerPass=" + PrinterPass);
    f.println("refreshRate=" + String(minutesBetweenDataRefresh));
    f.println("themeColor=" + themeColor);
    f.println("www_username=" + String(www_username));
    f.println("www_password=" + String(www_password));
    f.println("DISPLAYCLOCK=" + String(DISPLAYCLOCK));
    f.println("is24hour=" + String(IS_24HOUR));
    f.println("isWeather=" + String(DISPLAYWEATHER));
    f.println("weatherKey=" + WeatherApiKey);
    f.println("CityID=" + String(CityIDs[0]));
    f.println("isMetric=" + String(IS_METRIC));
  }
  f.close();
  readSettings();
  timeClient.setUtcOffset(UtcOffset);
}

void readSettings() {
  if (SPIFFS.exists(CONFIG) == false) {
    Serial.println("Settings File does not yet exists.");
    writeSettings();
    return;
  }
  File fr = SPIFFS.open(CONFIG, "r");
  String line;
  while(fr.available()) {
    line = fr.readStringUntil('\n');

    if (line.indexOf("UtcOffset=") >= 0) {
      UtcOffset = line.substring(line.lastIndexOf("UtcOffset=") + 10).toFloat();
      Serial.println("UtcOffset=" + String(UtcOffset));
    }
    if (line.indexOf("PrinterHostName=") >= 0) {
      PrinterHostName = line.substring(line.lastIndexOf("PrinterHostName=") + 16);
      PrinterHostName.trim();
      Serial.println("Printer Host or IP=" + PrinterHostName);
    }
    if (line.indexOf("printerPass=") >= 0) {
      PrinterPass = line.substring(line.lastIndexOf("printerPass=") + 12);
      PrinterPass.trim();
      Serial.println("PrinterPass=" + PrinterPass);
    }
    if (line.indexOf("refreshRate=") >= 0) {
      minutesBetweenDataRefresh = line.substring(line.lastIndexOf("refreshRate=") + 12).toInt();
      Serial.println("minutesBetweenDataRefresh=" + String(minutesBetweenDataRefresh));
    }
    if (line.indexOf("themeColor=") >= 0) {
      themeColor = line.substring(line.lastIndexOf("themeColor=") + 11);
      themeColor.trim();
      Serial.println("themeColor=" + themeColor);
    }
    if (line.indexOf("www_username=") >= 0) {
      String temp = line.substring(line.lastIndexOf("www_username=") + 13);
      temp.trim();
      temp.toCharArray(www_username, sizeof(temp));
      Serial.println("www_username=" + String(www_username));
    }
    if (line.indexOf("www_password=") >= 0) {
      String temp = line.substring(line.lastIndexOf("www_password=") + 13);
      temp.trim();
      temp.toCharArray(www_password, sizeof(temp));
      Serial.println("www_password=" + String(www_password));
    }
    if (line.indexOf("DISPLAYCLOCK=") >= 0) {
      DISPLAYCLOCK = line.substring(line.lastIndexOf("DISPLAYCLOCK=") + 13).toInt();
      Serial.println("DISPLAYCLOCK=" + String(DISPLAYCLOCK));
    }
    if (line.indexOf("is24hour=") >= 0) {
      IS_24HOUR = line.substring(line.lastIndexOf("is24hour=") + 9).toInt();
      Serial.println("IS_24HOUR=" + String(IS_24HOUR));
    }
    if (line.indexOf("isWeather=") >= 0) {
      DISPLAYWEATHER = line.substring(line.lastIndexOf("isWeather=") + 10).toInt();
      Serial.println("DISPLAYWEATHER=" + String(DISPLAYWEATHER));
    }
    if (line.indexOf("weatherKey=") >= 0) {
      WeatherApiKey = line.substring(line.lastIndexOf("weatherKey=") + 11);
      WeatherApiKey.trim();
      Serial.println("WeatherApiKey=" + WeatherApiKey);
    }
    if (line.indexOf("CityID=") >= 0) {
      CityIDs[0] = line.substring(line.lastIndexOf("CityID=") + 7).toInt();
      Serial.println("CityID: " + String(CityIDs[0]));
    }
    if (line.indexOf("isMetric=") >= 0) {
      IS_METRIC = line.substring(line.lastIndexOf("isMetric=") + 9).toInt();
      Serial.println("IS_METRIC=" + String(IS_METRIC));
    }
  }
  fr.close();
  PrinterClient.updatePrinterClient(PrinterHostName, PrinterPass);
  weatherClient.updateWeatherApiKey(WeatherApiKey);
  weatherClient.setMetric(IS_METRIC);
  weatherClient.updateCityIdList(CityIDs, 1);
  timeClient.setUtcOffset(UtcOffset);
}

int getMinutesFromLastRefresh() {
  int minutes = (timeClient.getCurrentEpoch() - lastEpoch) / 60;
  return minutes;
}

int getMinutesFromLastDisplay() {
  int minutes = (timeClient.getCurrentEpoch() - displayOffEpoch) / 60;
  return minutes;
}

// Toggle on and off the display if user defined times
void checkDisplay() {
  if (!displayOn && DISPLAYCLOCK) {
    enableDisplay(true);
  }
  if (displayOn && !(PrinterClient.isPrinting()) && !DISPLAYCLOCK) {
    // Put Display to sleep
    display.clear();
    display.display();
    display.setFont(ArialMT_Plain_16);
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.setContrast(255); // default is 255
    display.drawString(64, 5, "Printer Offline\nSleep Mode...");
    display.display();
    delay(5000);
    enableDisplay(false);
    Serial.println("Printer is offline going down to sleep...");
    return;    
  } else if (!displayOn && !DISPLAYCLOCK) {
    if (PrinterClient.isPrinting()) {
      // Wake the Screen up
      enableDisplay(true);
      display.clear();
      display.display();
      display.setFont(ArialMT_Plain_16);
      display.setTextAlignment(TEXT_ALIGN_CENTER);
      display.setContrast(255); // default is 255
      display.drawString(64, 5, "Printer Online\nWake up...");
      display.display();
      Serial.println("Printer is online waking up...");
      //delay(5000);
      return;
    }
  } else if (DISPLAYCLOCK) {
    if (!PrinterClient.isPrinting() && !isClockOn) {
      Serial.println("Clock Mode is turned on.");
      if (!DISPLAYWEATHER) {
        ui.disableAutoTransition();
        ui.setFrames(clockFrame, 1);
        clockFrame[0] = drawClock;
      } else {
        ui.enableAutoTransition();
        ui.setFrames(clockFrame, 2);
        clockFrame[0] = drawClock;
        clockFrame[1] = drawWeather;
      }
      ui.setOverlays(clockOverlay, numberOfOverlays);
      isClockOn = true;
    } else if (PrinterClient.isPrinting() && isClockOn) {
      Serial.println("Duet Monitor is active.");
      ui.setFrames(frames, numberOfFrames);
      ui.setOverlays(overlays, numberOfOverlays);
      ui.enableAutoTransition();
      isClockOn = false;
    }
  }
}

void enableDisplay(boolean enable) {
  displayOn = enable;
  if (enable) {
    if (getMinutesFromLastDisplay() >= minutesBetweenDataRefresh) {
      // The display has been off longer than the minutes between refresh -- need to get fresh data
      lastEpoch = 0; // this should force a data pull
      displayOffEpoch = 0;  // reset
    }
    display.displayOn();
    Serial.println("Display was turned ON: " + timeClient.getFormattedTime());
  } else {
    display.displayOff();
    Serial.println("Display was turned OFF: " + timeClient.getFormattedTime());
    displayOffEpoch = lastEpoch;
  }
}
