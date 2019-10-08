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

#include "PrinterClient.h"
int    myPort = 80;

PrinterClient::PrinterClient(String server, String pass) {
  updatePrinterClient(server, pass);
}

void PrinterClient::updatePrinterClient(String server, String pass) {
  server.toCharArray(myServer, 100);
}

boolean PrinterClient::validate() {
  boolean rtnValue = false;
  printerData.error = "";
  if (String(myServer) == "") {
    printerData.error += "Server name or address is required; ";
  }
  if (printerData.error == "") {
    rtnValue = true;
  }
  return rtnValue;
}

WiFiClient PrinterClient::getSubmitRequest(String apiGetData) {
  WiFiClient printClient;
  printClient.setTimeout(5000);

  Serial.println("Getting Printer Data from " + String(myServer));
  Serial.println(apiGetData);
  result = "";
  if (printClient.connect(myServer, 80)) {  //starts client connection, checks for connection
    printClient.println(apiGetData);
    printClient.println("Host: " + String(myServer) + ":" + String(myPort));
    // Password??
    printClient.println("User-Agent: ArduinoWiFi/1.1");
    printClient.println("Connection: close");
    if (printClient.println() == 0) {
      Serial.println("Connection to " + String(myServer) + ":" + String(myPort) + " failed.");
      Serial.println();
      resetPrintData();
      printerData.error = "Connection to " + String(myServer) + ":" + String(myPort) + " failed.";
      return printClient;
    }
  } 
  else {
    Serial.println("Connection to Printer failed: " + String(myServer) + ":" + String(myPort)); //error message if no client connect
    Serial.println();
    resetPrintData();
    printerData.error = "Connection to Printer failed: " + String(myServer) + ":" + String(myPort);
    return printClient;
  }

  // Check HTTP status
  char status[128] = {0};
  printClient.readBytesUntil('\r', status, sizeof(status));
  if (strstr(status, "HTTP/1.1 200 OK") != 0 && strstr(status, "HTTP/1.1 409 CONFLICT") != 0) {
    Serial.print(F("Unexpected response: "));
    Serial.println(status);
    printerData.status = "Bad HTTP Response";
    printerData.error = "Response: " + String(status);
    return printClient;
  }

  // Skip HTTP headers
  if (!(printClient.find("\r\n\r\n"))) {
    Serial.println(F("Invalid response - no EOH found"));
    Serial.println(printClient);
    printerData.error = "Invalid response - no EOh found from " + String(myServer) + ":" + String(myPort);
    printerData.status = "Invalid HTTP Headers";
  }

  return printClient;
}

void PrinterClient::getPrinterJobResults() {
  if (!validate()) {
    return;
  }
  String apiGetData = "GET /rr_status?type=3 HTTP/1.1";

  WiFiClient printClient = getSubmitRequest(apiGetData);

  if (printerData.error != "") {
    return;
  }
  
  const size_t bufferSize = JSON_OBJECT_SIZE(1) + JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(3) + 2*JSON_OBJECT_SIZE(5) + JSON_OBJECT_SIZE(6) + 710;
  DynamicJsonBuffer jsonBuffer(bufferSize);

  // Parse JSON object
  JsonObject& root = jsonBuffer.parseObject(printClient);
  if (!root.success()) {
    Serial.println("Printer Data Parsing failed: " + String(myServer) + ":" + String(myPort));
    printerData.error = "Printer Data Parsing failed: " + String(myServer) + ":" + String(myPort);
    printerData.status = "Failed JSON Parse";
    return;
  }
  
  printerData.averagePrintTime = (const char*)root["job"]["averagePrintTime"];
  //printerData.estimatedPrintTime = (const char*)root["job"]["estimatedPrintTime"];
  //printerData.fileName = (const char*)root["job"]["file"]["name"];
  //printerData.fileSize = (const char*)root["job"]["file"]["size"];
  //printerData.lastPrintTime = (const char*)root["job"]["lastPrintTime"];
  printerData.progressCompletion = (const char*)root["fractionPrinted"];
  printerData.progressFilepos = (const char*)root["progress"]["filepos"];
  printerData.progressPrintTime = (const char*)root["printDuration"];
  printerData.progressPrintTimeLeft = (const char*)root["timesLeft"]["filament"];
  printerData.filamentLength = (const char*)root["extr"][0];
  printerData.status = (const char*)root["status"];

  Serial.println("Status: " + printerData.status);

  //**** get the Printer Temps and Stat
  apiGetData = "GET /rr_status?type=3 HTTP/1.1";
  printClient = getSubmitRequest(apiGetData);
  if (printerData.error != "") {
    return;
  }
  const size_t bufferSize2 = 3*JSON_OBJECT_SIZE(2) + 2*JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(9) + 300;
  DynamicJsonBuffer jsonBuffer2(bufferSize2);

  // Parse JSON object
  JsonObject& root2 = jsonBuffer2.parseObject(printClient);
  if (!root2.success()) {
    printerData.isPrinting = false;
    printerData.toolTemp = "";
    printerData.toolTargetTemp = "";
    printerData.bedTemp = "";
    printerData.bedTargetTemp = "";
    return;
  }

  String printing = (const char*)root2["status"];
  if ((printing == "P") || (printing == "S") || (printing == "M")) {  // P = Printing;  S = Paused while printing; M = Simulating
    printerData.isPrinting = true;
  } else {
    printerData.isPrinting = false;
  }
    
  printerData.toolTemp = (const char*)root2["temps"]["current"][1];
  printerData.bedTemp = (const char*)root2["temps"]["bed"]["current"];

  if (isPrinting()) {
    Serial.println("Status: " + printerData.status + " " + printerData.fileName + "(" + printerData.progressCompletion + "%)" + " " + printerData.progressPrintTime);
  }
  
  printClient.stop(); //stop client
}

// Reset all PrinterData
void PrinterClient::resetPrintData() {
  printerData.averagePrintTime = "";
  printerData.estimatedPrintTime = "";
  printerData.fileName = "";
  printerData.fileSize = "";
  printerData.lastPrintTime = "";
  printerData.progressCompletion = "";
  printerData.progressFilepos = "";
  printerData.progressPrintTime = "";
  printerData.progressPrintTimeLeft = "";
  printerData.status = "";
  printerData.toolTemp = "";
  printerData.toolTargetTemp = "";
  printerData.filamentLength = "";
  printerData.bedTemp = "";
  printerData.bedTargetTemp = "";
  printerData.isPrinting = false;
  printerData.error = "";
}

String PrinterClient::getAveragePrintTime(){
  return printerData.averagePrintTime;
}

String PrinterClient::getEstimatedPrintTime() {
  return printerData.estimatedPrintTime;
}

String PrinterClient::getFileName() {
  return printerData.fileName;
}

String PrinterClient::getFileSize() {
  return printerData.fileSize;
}

String PrinterClient::getLastPrintTime(){
  return printerData.lastPrintTime;
}

String PrinterClient::getProgressCompletion() {
  return String(printerData.progressCompletion.toInt());
}

String PrinterClient::getProgressFilepos() {
  return printerData.progressFilepos;  
}

String PrinterClient::getProgressPrintTime() {
  return printerData.progressPrintTime;
}

String PrinterClient::getProgressPrintTimeLeft() {
  String rtnValue = printerData.progressPrintTimeLeft;
  if (getProgressCompletion() == "100") {
    rtnValue = "0"; // Print is done so this should be 0 this is a fix for OctoPrint
  }
  return rtnValue;
}

String PrinterClient::getStatus() {
  return printerData.status;
}

boolean PrinterClient::isPrinting() {
  return printerData.isPrinting;
}

String PrinterClient::getTempBedActual() {
  return printerData.bedTemp;
}

String PrinterClient::getTempBedTarget() {
  return printerData.bedTargetTemp;
}

String PrinterClient::getTempToolActual() {
  return printerData.toolTemp;
}

String PrinterClient::getTempToolTarget() {
  return printerData.toolTargetTemp;
}

String PrinterClient::getFilamentLength() {
  return printerData.filamentLength;
}

String PrinterClient::getError() {
  return printerData.error;
}

String PrinterClient::getValueRounded(String value) {
  float f = value.toFloat();
  int rounded = (int)(f+0.5f);
  return String(rounded);
}
