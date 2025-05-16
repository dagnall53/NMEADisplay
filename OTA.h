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
static bool INSTlogFileStarted = false;
static bool NMEAINSTlogFileStarted = false;

extern _sDisplay_Config Display_Config;

extern const char *Setupfilename;  
extern bool LoadConfiguration(const char *filename, _sDisplay_Config &config, _sWiFi_settings_Config &settings);
extern void SaveConfiguration(const char* filename, _sDisplay_Config& config, _sWiFi_settings_Config& settings);
extern _sWiFi_settings_Config Current_Settings;
extern void EEPROM_WRITE(_sDisplay_Config B,_sWiFi_settings_Config A);

extern void PrintJsonFile( const char* comment, const char* filename);
extern const char* VictronDevicesSetupfilename;  
extern _sMyVictronDevices  victronDevices;
// nb if victron or display settings are missing, '/save' will create them 
extern bool LoadVictronConfiguration(const char* filename, _sMyVictronDevices& config);
extern void SaveVictronConfiguration(const char* filename, _sMyVictronDevices& config);

extern bool LoadDisplayConfiguration(const char* filename, _MyColors& set);
extern void SaveDisplayConfiguration(const char* filename, _MyColors& set);

extern void SaveDisplayConfiguration(const char* filename, _MyColors& set);
extern const char* ColorsFilename ; 
extern _MyColors ColorSettings;
extern void showPicture(const char* name);
// extern Arduino_ST7701_RGBPanel *gfx ;  // declare the gfx structure so I can use GFX commands in Keyboard.cpp and here...
extern Arduino_RGB_Display *gfx;  //  change if alternate (not 'Arduino_RGB_Display' ) display !
extern void setFont(int);
extern const char soft_version[];
//const char *host = "NMEADisplay";
extern _sBoatData BoatData;
extern void WifiGFXinterrupt(int font, _sButton& button, const char* fmt, ...) ;
extern _sButton WifiStatus;
extern int Display_Page;
extern void Display(bool reset, int page);
WebServer server(80);
File uploadFile;
char SavedFile[30];
char InstLogFileName[25];
char NMEALogFileName[25];



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
// So I can modify the Display Panel Name! but also so that OTA works even without SD card present
//
/*the main html web page, with modified names etc    */
//prettified version 
String html_startws() {
String logs,filename;
String st =
 "<html> <head> <meta name='viewport' content='width=device-width, initial-scale=1.0'>"
    "<meta http-equiv='content-type' content='text/html;' charset='utf-8'>"
    "<title>NavDisplay</title>"
    "<style>"
     "body {background-color: white;color: black;text-align: center;font-family: sans-serif;color: #777;}"
       ".title {font-size: 2em; margin: 20px 0;text-align: center;}"
        ".version {text-align: center;margin-bottom: 10px;}"
        ".button-link {display: inline-block;padding: 0px 15px;background-color: #006;"
            "color: white;text-decoration: none;border-radius: 4px;margin: 5px 0;"
            "border: 1px solid #666;transition: background-color 0.3s;font-size: 15px;"
	          "background: #3498db; color: #fff;height: auto;}"
        ".button-link:hover {background-color: #666;}"
		        ".button-linkSmall {display: inline-block;padding: 0px 15px;background-color: #006;"
            "color: white;text-decoration: none;border-radius: 4px;margin: 5px 0;"
            "border: 1px solid #666;transition: background-color 0.3s;font-size: 10px;"
	          "background: #3498db; color: #fff;height: auto;}"
        "h1 {margin: 10px 0;font-size: 18px;}</style></head>"
        "<body>"
    "<img src='/v3small.jpg'><br>"
    "<div class='title'>";
    st+= String(Display_Config.PanelName);
    st+="</div>"
    "<div class='version'>";
     st+= String(soft_version);
    st+="</div>"
    "<h1><a class='button-link' href='http://";st+= String(Display_Config.PanelName); 
    st+=".local/Reset'>RESTART</a></h1>"
    "<h1><a class='button-link' href='http://";st+= String(Display_Config.PanelName); 
    st+=".local/OTA'>OTA UPDATE</a></h1>"
    "<h1><a class='button-link' href='http://";st+= String(Display_Config.PanelName); 
    st+=".local/edit/index.htm'>SD Files Editor</a></h1><br>"
	  "<div class='version'>Saved Log files on SD </div>";
  File dir = SD.open("/logs");
  for (int cnt = 0; true; ++cnt) {
    File entry = dir.openNextFile();
    Serial.println(entry.path());
    if (!entry) { break; }
    filename=String(entry.path());
  st+= "<h1 ><a class='button-linkSmall' href='http://";
  st+= String(Display_Config.PanelName);
  st+= ".local";st+=filename;
  st+="'> ";
  st+=" View ";st+=filename; st+="</a></h1>" ;
    entry.close();
  }
st+="</body></html> ";
return st;
}





/* Style  Noted: Arduino will mis-colour some parts*/

String style =
  "<style>#file-input,input{width:100%;height:44px;border-radius:4px;margin:10px auto;font-size:15px}"
  "input{background:#f1f1f1;border:0;padding:0 15px}body{background:#3498db;font-family:sans-serif;font-size:14px;color:#777}"
  "#file-input{padding:0;border:1px solid #ddd;line-height:44px;text-align:left;display:block;cursor:pointer}"
  "#bar,#prgbar{background-color:#f1f1f1;border-radius:10px}#bar{background-color:#3498db;width:0%;height:10px}"
  "form{background:#fff;max-width:258px;margin:75px auto;padding:30px;border-radius:5px;text-align:center}"
  ".btn{background:#3498db;color:#fff;cursor:pointer}</style>";

/* OTA Server Index Page */
// using local copy of the javascript ?..
String serverIndex() {
  String st;
  if (SD.exists("/edit/jquery.min.js")) {Serial.println("using local js");st="<script src='/edit/jquery.min.js'></script>";} 
  else{Serial.println("using Internet js"); st="<script src='https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js'></script>"; }
     
  /*"<script src='https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js'></script>"*/
  /*"<script src='/edit/jquery.min.js'></script>"*/
  st+="<form method='POST' action='#' enctype='multipart/form-data' id='upload_form'>"
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
  return st;
}




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
    //will not get here now!
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
    strcpy(SavedFile,upload.filename.c_str());
     
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
    Serial.print(upload.totalSize);
    Serial.print(" filename: <");Serial.print(SavedFile); Serial.print("> lookfor:eg. <");
    Serial.print(ColorsFilename);Serial.println(">");
    //Add equivalent to web initiated save here if the upload.filename filename is the config.txt or vconfig.txt?
    // so that the saved settings are actually used immediately by the display - rather than waiting for the save button or
    //  power off.  
    //File will have been saved, so 'load' it into the actual code. 
    // remember strcmp returns zero if equal! or a number of the place the characters do not match
    if (strcmp(SavedFile,Setupfilename)==0) {Serial.println("Re-Loading Config");
      LoadConfiguration(Setupfilename, Display_Config, Current_Settings);
      SaveConfiguration(Setupfilename, Display_Config, Current_Settings);// and re-save, so that any new format to the file is saved
      delay(50);Display(true,Display_Config.Start_Page);delay(50);
      }
    if (strcmp(SavedFile,VictronDevicesSetupfilename)==0) {Serial.println("Re-Loading Victron Config");
      LoadVictronConfiguration(VictronDevicesSetupfilename,victronDevices);
      SaveVictronConfiguration(VictronDevicesSetupfilename,victronDevices); // and re-save, so that any new format to the file is saved
      delay(50);Display(true,Display_Page);delay(50);}
    if (strcmp(SavedFile,ColorsFilename)==0) {Serial.println("Re-Loading Colour Config");
      LoadDisplayConfiguration(ColorsFilename,ColorSettings);
      SaveDisplayConfiguration(ColorsFilename,ColorSettings);// and re-save, so that any new format to the file is saved
      delay(50);Display(true,Display_Page);delay(50);}
    SavedFile[0]=0;
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
  Serial.print(" handling: server.uri:<");
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
void SetupWebstuff() {
  if (MDNS.begin(Display_Config.PanelName)) {
    MDNS.addService("http", "tcp", 80);
    Serial.println("MDNS responder started");
    Serial.print("You can now connect to http://");
    Serial.print(Display_Config.PanelName);
    Serial.println(".local");
    WifiGFXinterrupt(8, WifiStatus, "MDNS responder started\nconnect to\n http://%s.local",Display_Config.PanelName);
  }
  //**************
  server.on("/", HTTP_GET, []() {
    Serial.println(" handling  root");
    WifiGFXinterrupt(8, WifiStatus, "Running Webserver");
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
    handleRoot();
    if (LoadConfiguration(Setupfilename, Display_Config, Current_Settings)) {EEPROM_WRITE(Display_Config,Current_Settings);}// stops overwrite with bad JSON data!! 
    // Victron is never set up by the touchscreen only via SD editor so NO SaveVictronConfiguration(VictronDevicesSetupfilename,victronDevices);// save config with bytes ??
    WifiGFXinterrupt(8, WifiStatus, "RESTARTING");
    handleRoot(); // hopefully this will prevent the webbrowser keeping the/reset active and auto reloading last web command (and thus resetting!) ? 
    server.sendHeader("Connection", "close");delay(150);
    WiFi.disconnect();
    ESP.restart();
  });
  server.on("/Save", HTTP_GET, []() {
    handleRoot();
    if (LoadConfiguration(Setupfilename, Display_Config, Current_Settings)) {Serial.println("***Updating EEPROM from ");EEPROM_WRITE(Display_Config,Current_Settings);}// stops overwrite with bad JSON data!! 
    if (LoadVictronConfiguration(VictronDevicesSetupfilename,victronDevices)) { 
      PrintJsonFile(" Check Updated Victron settings after Web initiated SAVE ",VictronDevicesSetupfilename); Serial.println("***Updated Victron data settings");}
    if (LoadDisplayConfiguration(ColorsFilename,ColorSettings)){
    Serial.println(" USING JSON for Colours data settings");
    }else { Serial.println("\n\n***FAILED TO GET Colours JSON FILE****\n**** SAVING DEFAULT and Making File on SD****\n\n");
    SaveDisplayConfiguration(ColorsFilename,ColorSettings);// should write a default file if it was missing?
    }
    delay(50);Display(true,Display_Page);delay(50);
  });

  server.on("/OTA", HTTP_GET, []() {
    WifiGFXinterrupt(8, WifiStatus, "Ready for OTA");
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", serverIndex());
  });
  server.on("/ota", HTTP_GET, []() {
    WifiGFXinterrupt(8, WifiStatus, "Ready for OTA");
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", serverIndex());
  });


  /*handling uploading firmware file */
  server.on(
    "/update", HTTP_POST, []() {
      server.sendHeader("Connection", "close");
      server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
        WiFi.disconnect();
      ESP.restart();
    },
    []() {
      HTTPUpload &upload = server.upload();
      if (upload.status == UPLOAD_FILE_START) {
        setFont(8);
        gfx->setTextColor(BLACK); 
        gfx->fillScreen(BLUE);
        showPicture("/loading.jpg");
        
        
        gfx->setCursor(0, 40);
        gfx->setTextWrap(true);
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




void StartInstlogfile() {
  INSTlogFileStarted = false;
  if (!hasSD) {  return; }
  // If the data.txt file doesn't exist
  // Create a file on the SD card and write the data labels
  if (BoatData.GPSDate == 0) { return; }
  if (BoatData.GPSDate == NMEA0183DoubleNA) { return; }  // and check for NMEA0183DoubleNA
  //We have a date so we can use this for the file name!
  // Serial.printf("  ***** LOG FILE DEBUG ***  use: <%6i>  to make name..  ",int(BoatData.GPSDate));
  snprintf(InstLogFileName, 25, "/logs/%6i.log", int(BoatData.GPSDate));
  //  Serial.printf("  <%s> \n",InstLogFileName);
  File file = SD.open(InstLogFileName);
  if (!file) {
    //Serial.println("File doesn't exist");
    INSTlogFileStarted = true;
    Serial.printf("Creating <%s> Instrument LOG file. and header..\n", InstLogFileName);
    // data will be added by a see the  LOG( fmt ...) in the main loop at 5 sec intervals
    /*    int(BoatData.GPSTime) / 3600, (int(BoatData.GPSTime) % 3600) / 60, (int(BoatData.GPSTime) % 3600) % 60,
        BoatData.STW.data,  BoatData.WaterDepth.data, BoatData.WindSpeedK.data,BoatData.WindAngleApp);
        */
    writeFile(SD, InstLogFileName, "LOG data headings\r\n Local Time ,STW ,MagHdg, SOG, COG,Depth ,Windspeed,WindAngleApp\r\n");
    file.close();
    return;
  } else {
     INSTlogFileStarted = true;
     Serial.println("Log File already exists.. appending");
  }
  file.close();
}

void StartNMEAlogfile() {
  if (!hasSD) {  NMEAINSTlogFileStarted = false;return; }
  // If the data.txt file doesn't exist
  // Create a (FIXED NAME!) file on the SD card and write the data labels
  File file = SD.open("/logs/NMEA.log");
  if (!file) {
    //Serial.println("File doens't exist");
    NMEAINSTlogFileStarted = true;
    Serial.printf("Creating NMEA LOG file. and header..\n");
    writeFile(SD, "/logs/NMEA.log", "NMEA data headings\r\nTime(s): Source:NMEA......\r\n");
    file.close();
    return;
  } else {NMEAINSTlogFileStarted = true;
     Serial.println("NMEA log File already exists.. appending");
  }
  file.close();
}

void NMEALOG(const char *fmt, ...) {
  if (!NMEAINSTlogFileStarted) {
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

void INSTLOG(const char *fmt, ...) {
  if (!INSTlogFileStarted) {
    StartInstlogfile();
    return;
  }
  static char msg[300] = { '\0' };  // used in message buildup
  va_list args;
  va_start(args, fmt);
  vsnprintf(msg, 128, fmt, args);
  va_end(args);
  int len = strlen(msg);
  // Serial.printf("  Logging to:<%s>", InstLogFileName);
  // Serial.print("  Log  data: ");
  // Serial.println(msg);
  appendFile(SD, InstLogFileName, msg);
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