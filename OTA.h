#ifndef _OTA_H_
#define _OTA_H_
/*
 BASED ON  SDWebServer - Example WebServer with SD Card backend for esp8266

  Copyright (c) 2015 Hristo Gochkov. All rights reserved.
  This file is part of the WebServer library for Arduino environment.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

  Have a FAT Formatted SD Card connected to the SPI port of the ESP8266
  The web root is the SD Card root folder
  File extensions with more than 3 charecters are not supported by the SD Library
  File Names longer than 8 charecters will be truncated by the SD library, so keep filenames shorter
  index.htm is the default index (works on subfolders as well)

  upload the contents of SdRoot to the root of the SDcard and access the editor by going to 
  http://nmeadisplay.local/edit
  To retrieve the contents of SDcard, visit http://nmeadisplay.local/list?dir=/
      dir is the argument that needs to be passed to the function PrintDirectory via HTTP Get request.
  NB  in my version nmeadisplay is settable in the config.txt, so you may need to access 192.168.4.1 if accessing the AP.. 

*/


#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Update.h>
#include <Arduino_GFX_Library.h>
#include "Structures.h"

#include <SPI.h>
#include <SD.h>

extern bool hasSD;
static bool logFileStarted = false;
static bool NMEAlogFileStarted = false;

extern JSONCONFIG Display_Config;
extern const char *Setupfilename;  // <- SD library uses 8.3 filenames
extern bool LoadConfiguration(const char *filename, JSONCONFIG &config, MySettings &settings);
extern MySettings Current_Settings;
extern void EEPROM_WRITE(JSONCONFIG B,MySettings A);

// extern Arduino_ST7701_RGBPanel *gfx ;  // declare the gfx structure so I can use GFX commands in Keyboard.cpp and here...
extern Arduino_RGB_Display *gfx;  //  change if alternate (not 'Arduino_RGB_Display' ) display !
extern void setFont(int);
extern const char soft_version[];
//const char *host = "NMEADisplay";
extern tBoatData BoatData;

WebServer server(80);
File uploadFile;

// for placing startws here and not on SD  -
// So I can modify the Display Panel Name! but also so that OTA works even without SD card present
//

// Slightly more flexible way of defining page.. allows if statements ..required for changed displayname.. 
String html_Question() {
  String st = "<!DOCTYPE html>\r\n";
  st += "<html><head>";
  st += "<title>NavDisplay ";
  st += String(soft_version);
  st += "</title>";
  st += " </head>";
  st += "<body><h1 ><a>This device's panel name is ";
  st += String(Display_Config.PanelName);
  st += "</h1><br>\r\n</a><h1>Software :";
  st += String(soft_version);
  st += "</h1><br> I may add user help instructions here:<br>";
  st += "</body></html>";
  return st;
}
/*the main html web page, with modified names etc    */
String html_startws() {
String st =
  "<!DOCTYPE html>\r\n<html><head> <meta http-equiv="
  "content-type"
  " content="
  "text/html; charset=utf-8"
  ">"
  "<title>NavDisplay</title>"
  "<style>"
  "body {background-color:black;color:white;}"
  "</style>"
  "</head><body>"
  "<img src='/v3small.jpg' /><br>"
  "<h1 ><a>";
  st+= String(Display_Config.PanelName);
  st+= "</h1><br>"
       " <h2>Software :";
  st+= String(soft_version);
  st+= "</h2></a>"
  "<h1 ><a style='color:white;' href='http://";
  st+= String(Display_Config.PanelName);
  st+= ".local/edit/index.htm'>SD File Access</a></h1>"
  "  <h1 >  <a style='color:white;' href='http://";
  st+= String(Display_Config.PanelName);
  st += ".local/OTA'>OTA Update</a></h1>"
  "  <h1 >  <a style='color:white;' href='http://";
  st+= String(Display_Config.PanelName);
  st += ".local/Reset'>Save/Reset</a></h1>"
  "</body></html>";
  return st;
  }

/* Style  Noted: Arduino will mis-colour some parts such as %;he */
String style =
  "<style>#file-input,input{width:100%;height:44px;border-radius:4px;margin:10px auto;font-size:15px}"
  "input{background:#f1f1f1;border:0;padding:0 15px}body{background:#3498db;font-family:sans-serif;font-size:14px;color:#777}"
  "#file-input{padding:0;border:1px solid #ddd;line-height:44px;text-align:left;display:block;cursor:pointer}"
  "#bar,#prgbar{background-color:#f1f1f1;border-radius:10px}#bar{background-color:#3498db;width:0%;height:10px}"
  "form{background:#fff;max-width:258px;margin:75px auto;padding:30px;border-radius:5px;text-align:center}"
  ".btn{background:#3498db;color:#fff;cursor:pointer}</style>";

/* OTA Server Index Page */
String serverIndex =
  "<script src='https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js'></script>"
  "<form method='POST' action='#' enctype='multipart/form-data' id='upload_form'>"
  "<input type='file' name='update' id='file' onchange='sub(this)' style=display:none>"
  "<h1>OTA Interface for NMEA DISPLAY</h1>"
  "<br><center>"
  + String(soft_version) + "</center> <br>"
                           "<label id='file-input' for='file'>   Choose file...</label>"
                           "<input type='submit' class=btn value='Update'>"
                           "<br><br>"
                           "<div id='prg'></div>"
                           "<br><div id='prgbar'><div id='bar'></div></div><br></form>"
                           "<script>"
                           "function sub(obj){"
                           "var fileName = obj.value.split('\\\\');"
                           "document.getElementById('file-input').innerHTML = '   '+ fileName[fileName.length-1];"
                           "};"
                           "$('form').submit(function(e){"
                           "e.preventDefault();"
                           "var form = $('#upload_form')[0];"
                           "var data = new FormData(form);"
                           "$.ajax({"
                           "url: '/update',"
                           "type: 'POST',"
                           "data: data,"
                           "contentType: false,"
                           "processData:false,"
                           "xhr: function() {"
                           "var xhr = new window.XMLHttpRequest();"
                           "xhr.upload.addEventListener('progress', function(evt) {"
                           "if (evt.lengthComputable) {"
                           "var per = evt.loaded / evt.total;"
                           "$('#prg').html('progress: ' + Math.round(per*100) + '%');"
                           "$('#bar').css('width',Math.round(per*100) + '%');"
                           "}"
                           "}, false);"
                           "return xhr;"
                           "},"
                           "success:function(d, s) {"
                           "console.log('success!') "
                           "},"
                           "error: function (a, b, c) {"
                           "}"
                           "});"
                           "});"
                           "</script>"
  + style;




void returnOK() {
  server.send(200, "text/plain", "");
}

void returnFail(String msg) {
  server.send(500, "text/plain", msg + "\r\n");
}

void handleRoot() {
  Serial.println(" sending local html version");
  server.send(200, "text/html", html_startws() + "\r\n");
}

bool loadFromSdCard(String path) {
  String dataType = "text/plain";
  //  previous version used just startws.htm  html from SD card.
  if (path.endsWith("/")) {  // send our local version with modified path!
    handleRoot();
    return true;
    //wilnot get here now!
    path += "startws.htm";  // start our html file from  the SD !
  }

  if (path.endsWith(".src")) {
    path = path.substring(0, path.lastIndexOf("."));
  } else if (path.endsWith(".htm")) {
    dataType = "text/html";
  } else if (path.endsWith(".css")) {
    dataType = "text/css";
  } else if (path.endsWith(".js")) {
    dataType = "application/javascript";
  } else if (path.endsWith(".png")) {
    dataType = "image/png";
  } else if (path.endsWith(".gif")) {
    dataType = "image/gif";
  } else if (path.endsWith(".jpg")) {
    dataType = "image/jpeg";
  } else if (path.endsWith(".ico")) {
    dataType = "image/x-icon";
  } else if (path.endsWith(".xml")) {
    dataType = "text/xml";
  } else if (path.endsWith(".pdf")) {
    dataType = "application/pdf";
  } else if (path.endsWith(".zip")) {
    dataType = "application/zip";
  } else if (path.endsWith(".mp3")) {
    dataType = "audio/mpeg";
  }

  File dataFile = SD.open(path.c_str());
  if (dataFile.isDirectory()) {
    path += "/index.htm";
    dataType = "text/html";
    dataFile = SD.open(path.c_str());
  }

  if (!dataFile) {
    return false;
  }

  if (server.hasArg("download")) {
    dataType = "application/octet-stream";
  }

  if (server.streamFile(dataFile, dataType) != dataFile.size()) {
    Serial.println("Sent less data than expected!");
  }

  dataFile.close();
  return true;
}

void handleFileUpload() {
  if (server.uri() != "/edit") {
    return;
  }
  HTTPUpload &upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    if (SD.exists((char *)upload.filename.c_str())) {
      SD.remove((char *)upload.filename.c_str());
    }
    uploadFile = SD.open(upload.filename.c_str(), FILE_WRITE);
    Serial.print("Upload: START, filename: ");
    Serial.println(upload.filename);
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (uploadFile) {
      uploadFile.write(upload.buf, upload.currentSize);
    }
    Serial.print("Upload: WRITE, Bytes: ");
    Serial.println(upload.currentSize);
  } else if (upload.status == UPLOAD_FILE_END) {
    if (uploadFile) {
      uploadFile.close();
    }
    Serial.print("Upload: END, Size: ");
    Serial.println(upload.totalSize);
  }
}

void deleteRecursive(String path) {
  File file = SD.open((char *)path.c_str());
  if (!file.isDirectory()) {
    file.close();
    SD.remove((char *)path.c_str());
    return;
  }

  file.rewindDirectory();
  while (true) {
    File entry = file.openNextFile();
    if (!entry) {
      break;
    }
    String entryPath = path + "/" + entry.name();
    if (entry.isDirectory()) {
      entry.close();
      deleteRecursive(entryPath);
    } else {
      entry.close();
      SD.remove((char *)entryPath.c_str());
    }
    yield();
  }

  SD.rmdir((char *)path.c_str());
  file.close();
}

void handleDelete() {
  if (server.args() == 0) {
    return returnFail("BAD ARGS");
  }
  String path = server.arg(0);
  if (path == "/" || !SD.exists((char *)path.c_str())) {
    returnFail("BAD PATH");
    return;
  }
  deleteRecursive(path);
  returnOK();
}

void handleCreate() {
  if (server.args() == 0) {
    return returnFail("BAD ARGS");
  }
  String path = server.arg(0);
  if (path == "/" || SD.exists((char *)path.c_str())) {
    returnFail("BAD PATH");
    return;
  }

  if (path.indexOf('.') > 0) {
    File file = SD.open((char *)path.c_str(), FILE_WRITE);
    if (file) {
      file.write(0);
      file.close();
    }
  } else {
    SD.mkdir((char *)path.c_str());
  }
  returnOK();
}

void printDirectory() {
  if (!server.hasArg("dir")) {
    return returnFail("BAD ARGS");
  }
  String path = server.arg("dir");
  if (path != "/" && !SD.exists((char *)path.c_str())) {
    return returnFail("BAD PATH");
  }
  File dir = SD.open((char *)path.c_str());
  path = String();
  if (!dir.isDirectory()) {
    dir.close();
    return returnFail("NOT DIR");
  }
  dir.rewindDirectory();
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/json", "");
  WiFiClient client = server.client();

  server.sendContent("[");
  for (int cnt = 0; true; ++cnt) {
    File entry = dir.openNextFile();
    if (!entry) {
      break;
    }

    String output;
    if (cnt > 0) {
      output = ',';
    }

    output += "{\"type\":\"";
    output += (entry.isDirectory()) ? "dir" : "file";
    output += "\",\"name\":\"";
    output += entry.path();
    output += "\"";
    output += "}";
    server.sendContent(output);
    entry.close();
  }
  server.sendContent("]");
  dir.close();
}

void handleNotFound() {
  Serial.print(" handleNotFound: server.uri:<");
  Serial.print(server.uri());
  Serial.println(">");
  if (hasSD && loadFromSdCard(server.uri())) {
    return;
  }
  String message = "";
  if (!hasSD) {
    message += "SDCARD Not Detected\n\n";
  } else {
    message += " not understood\n\n";
  }
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " NAME:" + server.argName(i) + "\n VALUE:" + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);  //404 is the error message
  Serial.print(message);
}

void handleQuestion() {
  Serial.println(" sending handle Question");
  Serial.println(html_Question());
  server.sendContent(html_Question());
  server.sendContent("");
  server.client().stop();
}
void SetupOTA() {
  if (MDNS.begin(Display_Config.PanelName)) {
    MDNS.addService("http", "tcp", 80);
    Serial.println("MDNS responder started");
    Serial.print("You can now connect to http://");
    Serial.print(Display_Config.PanelName);
    Serial.println(".local");
  }
  //**************
  server.on("/", HTTP_GET, []() {
    Serial.println(" handling  root");
    handleRoot();
  });

  server.on("/Q", handleQuestion);
  //server.on("/", HTTP_GET,handleRoot); //??
  server.on("/list", HTTP_GET, printDirectory);
  server.on("/edit", HTTP_DELETE, handleDelete);
  server.on("/edit", HTTP_PUT, handleCreate);
  server.on(
    "/edit", HTTP_POST, []() {
      returnOK();
    },
    handleFileUpload);
  server.onNotFound(handleNotFound);

  server.on("/Reset", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    if (LoadConfiguration(Setupfilename, Display_Config, Current_Settings)) {EEPROM_WRITE(Display_Config,Current_Settings);}// stops overwrite with bad JSON data!! 
    delay(50);
    ESP.restart();
  });

  server.on("/OTA", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", serverIndex);
  });
  server.on("/ota", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", serverIndex);
  });


  /*handling uploading firmware file */
  server.on(
    "/update", HTTP_POST, []() {
      server.sendHeader("Connection", "close");
      server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
      ESP.restart();
    },
    []() {
      HTTPUpload &upload = server.upload();
      if (upload.status == UPLOAD_FILE_START) {
        setFont(10);
        gfx->setTextColor(WHITE);
        gfx->fillScreen(BLUE);
        gfx->setCursor(0, 40);
        gfx->printf("Update: %s\n", upload.filename.c_str());
        Serial.printf("Update: %s\n", upload.filename.c_str());
        Serial.printf("   current size: %u   total size %u\n", upload.currentSize, upload.totalSize);
        if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {  //start with max available size
          Update.printError(Serial);
        }
      } else if (upload.status == UPLOAD_FILE_WRITE) {
        /* flashing firmware to ESP*/
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
          Update.printError(Serial);
        }
        //monitor upload milestones (100k 200k 300k)?
        uint16_t chunk_size = 51200;
        static uint32_t next = 51200;
        if (upload.totalSize >= next) {

          gfx->printf("%dk ", next / 1024);
          Serial.printf("%dk ", next / 1024);
          next += chunk_size;
        }
      } else if (upload.status == UPLOAD_FILE_END) {
        if (Update.end(true)) {             //true to set the size to the current progress
          gfx->printf("Update Success:n");  // no point in sending size to display - it only flashes momentarily!
          Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
          delay(500);
        } else {
          Update.printError(Serial);
        }
      }
    });


  //*********** END of OTA stuff *****************


  server.begin();
  Serial.println("HTTP server started");
}

// from https://randomnerdtutorials.com/esp32-data-logging-temperature-to-microsd-card/
void writeFile(fs::FS &fs, const char *path, const char *message) {
  Serial.printf("Writing file: %s\n", path);

  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }
  if (file.print(message)) {
    //  Serial.println("File written");
  } else {
    Serial.println("Write failed");
  }
  file.close();
}

// Append data to the SD card (DON'T MODIFY THIS FUNCTION)
void appendFile(fs::FS &fs, const char *path, const char *message) {
  // Serial.printf("Appending to file: %s\n", path);

  File file = fs.open(path, FILE_APPEND);
  if (!file) {
    // Serial.println("Failed to open file for appending");
    return;
  }
  if (file.print(message)) {
    //  Serial.println("Message appended");
  } else {
    //  Serial.println("Append failed");
  }
  file.close();
}


char LogFileName[25];
char NMEALogFileName[25];

void Startlogfile() {
  if (!hasSD) { return; }
  // If the data.txt file doesn't exist
  // Create a file on the SD card and write the data labels
  if (BoatData.GPSDate == 0) { return; }
  if (BoatData.GPSDate == NMEA0183DoubleNA) { return; }  // and check for NMEA0183DoubleNA?
  //We have a date so we can use this for the file name!
  // Serial.printf("  ***** LOG FILE DEBUG ***  use: <%6i>  to make name..  ",int(BoatData.GPSDate));
  snprintf(LogFileName, 25, "/logs/%6i.log", int(BoatData.GPSDate));
  //  Serial.printf("  <%s> \n",LogFileName);
  File file = SD.open(LogFileName);
  if (!file) {
    //Serial.println("File doesn't exist");
    logFileStarted = true;
    Serial.printf("Creating <%s>LOG file. and header..\n", LogFileName);
    // data will be added by a see the  LOG( fmt ...) in the main loop at 5 sec intervals
    /*   LOG("TIME: %02i:%02i:%02i ,%4.2f ,%4.2f ,%4.2f ,%3.1f ,%4.0f ,%f ,%f \r\n",
        int(BoatData.GPSTime) / 3600, (int(BoatData.GPSTime) % 3600) / 60, (int(BoatData.GPSTime) % 3600) % 60,
        BoatData.STW.data, BoatData.SOG.data, BoatData.WaterDepth.data, BoatData.WindSpeedK.data,
        BoatData.COG.data,BoatData.MagHeading.data,
        BoatData.WindAngleApp.data, BoatData.Latitude.data, BoatData.Longitude.data);*/
    writeFile(SD, LogFileName, "NEW LOG data headings\r\nTime ,STW,SOG,Depth,Windspeed,WindAngleApp,COG,MagHeading,Lat,Long \r\n");
    file.close();
    return;
  } else {
    // Serial.println("File already exists");
  }
  file.close();
}

void StartNMEAlogfile() {
  if (!hasSD) { return; }
  // If the data.txt file doesn't exist
  // Create a (FIXED NAME!) file on the SD card and write the data labels
  File file = SD.open("/logs/NMEA.log");
  if (!file) {
    //Serial.println("File doens't exist");
    NMEAlogFileStarted = true;
    Serial.printf("Creating NMEA LOG file. and header..\n");
    writeFile(SD, "/logs/NMEA.log", "NMEA data headings\r\nTime(s): Source:NMEA......\r\n");
    file.close();
    return;
  } else {
    // Serial.println("File already exists");
  }
  file.close();
}

void NMEALOG(const char *fmt, ...) {
  if (!NMEAlogFileStarted) {
    StartNMEAlogfile();
    return;
  }
  static char msg[300] = { '\0' };  // used in message buildup
  va_list args;
  va_start(args, fmt);
  vsnprintf(msg, 128, fmt, args);
  va_end(args);
  int len = strlen(msg);
  // Serial.printf("  Logging to:<%s>", NMEALogFileName);
  // Serial.print("  Log  data: ");
  // Serial.println(msg);
  appendFile(SD, "/logs/NMEA.log", msg);
}

void LOG(const char *fmt, ...) {
  if (!logFileStarted) {
    Startlogfile();
    return;
  }
  static char msg[300] = { '\0' };  // used in message buildup
  va_list args;
  va_start(args, fmt);
  vsnprintf(msg, 128, fmt, args);
  va_end(args);
  int len = strlen(msg);
  // Serial.printf("  Logging to:<%s>", LogFileName);
  // Serial.print("  Log  data: ");
  // Serial.println(msg);
  appendFile(SD, LogFileName, msg);
}




// void ShowFreeSpace() {
//   // Calculate free space (volume free clusters * blocks per clusters / 2)
//   long lFreeKB = SD.vol()->freeClusterCount();
//   lFreeKB *= SD.vol()->blocksPerCluster()/2;

//   // Display free space
//   Serial.print("Free space: ");
//   Serial.print(lFreeKB);
//   Serial.println(" KB");
// }

#endif