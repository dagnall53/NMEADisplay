//*******************************************************************************
/*
Compiled and tested with ESp32 V2.0.17  V3has broken  some functions and this code will not work with V3!
GFX library for Arduino 1.5.5
Select "ESP32-S3 DEV Module"
Select PSRAM "OPI PSRAM" / enabled

16M flash 
8m with spiffs - 3Mb app +spiffs


Version 3 tests try to use 
#if ESP_ARDUINO_VERSION_MAJOR ==3 ? 

Schreibfaul1 esp32 audio needs V3 for V3 compiler:
or 2.0.0 for Version 2.0.17 -- its not cross compatible.  

GFX seems ok , but change the.h as noted: but Jpeg screen seems to flicker with GFX 1.6 and Version3.2.0 compiler. 
COMPILED AGAIN wITH 2.0.17 AND GFX 1.6 flicker less 

*/

#if ESP_ARDUINO_VERSION_MAJOR == 3  // hoping this #if will work in the called .cpp !!
#define UsingV3Compiler             // this #def DOES NOT WORK by itsself! it only affects .h not .cpp files  !! (v3 ESPnow is very different) directive to replace std::string with String for Version 3 compiler and also (?) other V3 incompatibilites
#endif



#include <NMEA0183.h>
#include <NMEA0183Msg.h>
#include <NMEA0183Messages.h>

//******  Wifi stuff

#include <WiFi.h>
#include <WiFiUdp.h>
//
#include <esp_now.h>
#include <esp_wifi.h>
#include "ESP_NOW_files.h"
#include "OTA.h"

//#define NumVictronDevices 10  // before structures
#include "Structures.h"

#include "aux_functions.h"
#include <Arduino_GFX_Library.h>
// Original version was for GFX 1.3.1 only. #include "GUITIONESP32-S3-4848S040_GFX_133.h"
#include "Esp32_4inch.h"  // defines GFX settings for GUITIONESP32-S3-4848S040!
//*********** for keyboard*************
#include "Keyboard.h"
#include "Touch.h"
TAMC_GT911 ts = TAMC_GT911(TOUCH_SDA, TOUCH_SCL, TOUCH_INT, TOUCH_RST, TOUCH_WIDTH, TOUCH_HEIGHT);

#include <EEPROM.h>
#include "FONTS/fonts.h"               // have to use reserved directory name src/ for arduino?
#include "FONTS/FreeSansBold6pt7b.h"   //font 7  9 high
#include "FONTS/FreeSansBold8pt7b.h"   //font 8  11 high
#include "FONTS/FreeSansBold12pt7b.h"  // font 9  18 pixels high
#include "FONTS/FreeSansBold18pt7b.h"  //font 10  27 pixels
#include "FONTS/FreeSansBold27pt7b.h"  // font 11 39 pixels
#include "FONTS/FreeSansBold40pt7b.h"  //font 12 59 pixels
#include "FONTS/FreeSansBold60pt7b.h"  //font 13  88 pixels




//For SD card (see display page -98 for test)
// allow comments in the JSON FILE
#define ARDUINOJSON_ENABLE_COMMENTS 1
#include <ArduinoJson.h>
#include <SD.h>  // pins set in GFX .h
#include "SPI.h"
#include "FS.h"

//jpeg
#include "JpegFunc.h"
#define JPEG_FILENAME_LOGO "/logo4.jpg"  // logo in jpg on sd card
#define AUDIO_FILENAME_START "/StartSound.mp3"
//audio

//v3 tests.. no audio until I fix the is2  ?  /
//now compiles using https://github.com/schreibfaul1/ESP32-audioI2S  v 3.0.13 (was using 2.0.0 with V2.0.17   esp32 compiler)

#include "Audio.h"

#include "VICTRONBLE.h"  //sets #ifndef Victronble_h



//const char soft_version[] = "Version 4.05";
//const char compile_date[] = __DATE__ " " __TIME__;
const char soft_version[] = "VERSION 4.31";

bool hasSD;



//************JSON SETUP STUFF to get setup parameters from the SD card to make it easy for user to change
// READ https://arduinojson.org/?utm_source=meta&utm_medium=library.properties
// *********************************************************************************************************
const char* Setupfilename = "/config.txt";  // <- SD library uses 8.3 filenames
// victron config structure for mac and keys (SD only not on eeprom)

int Num_Victron_Devices;
const char* VictronDevicesSetupfilename = "/vconfig.txt";  // <- SD library uses 8.3 filenames
_sMyVictronDevices victronDevices;

char VictronBuffer[2000];  // way to transfer results in a way similar to NMEA text.

// _sVictronData VictronData;  // Victron sensor Data values, int double etc for use in graphics

const char* ColorsFilename = "/colortest.txt";  // <- SD library uses 8.3 filenames?
_MyColors ColorSettings;

//set up Audio
Audio audio;


// some wifi stuff
//NB these may not be used -- I have tried to do some simplification
IPAddress udp_ap(0, 0, 0, 0);   // the IP address to send UDP data in SoftAP mode
IPAddress udp_st(0, 0, 0, 0);   // the IP address to send UDP data in Station mode
IPAddress sta_ip(0, 0, 0, 0);   // the IP address (or received from DHCP if 0.0.0.0) in Station mode
IPAddress gateway(0, 0, 0, 0);  // the IP address for Gateway in Station mode
IPAddress subnet(0, 0, 0, 0);   // the SubNet Mask in Station mode
boolean IsConnected = false;    // may be used in AP_AND_STA to flag connection success (got IP)
boolean AttemptingConnect;      // to note that WIFI.begin has been started
int NetworksFound;              // used for scan Networks result. Done at start up!
WiFiUDP Udp;
#define BufferLength 500
char nmea_1[BufferLength];    //serial
char nmea_U[BufferLength];    // NMEA buffer for UDP input port
char nmea_EXT[BufferLength];  // buffer for ESP_now received data

bool EspNowIsRunning = false;
char* pTOKEN;
int StationsConnectedtomyAP;

#define scansearchinterval 30000

// assists for wifigfx interrupt  box that shows status..  to help turn it off after a time
uint32_t WIFIGFXBoxstartedTime;
bool WIFIGFXBoxdisplaystarted;

//********** All boat data (instrument readings) are stored as double in a single structure:

_sBoatData BoatData;  // BoatData values, int double etc

bool dataUpdated;  // flag that Nmea Data has been updated

//**   structures and helper functions for my variables



//for MP3 player based on (but modified!) https://github.com/VolosR/MakePythonLCDMP3/blob/main/MakePythonLCDMP3.ino#L119
char File_List[20][20];  //array of 20 (20 long) file names for listing..
String runtimes[20];
int file_num = 0;

// _sWiFi_settings_Config (see structures.h) are the settings for the Display:
// If the structure is changed, be sure to change the Key (first figure) so that new defaults and struct can be set.
//                                                                                    LOG off NMEAlog Off
_sWiFi_settings_Config Default_Settings = { 17, "GUESTBOAT", "12345678", "2002", false, true, true, false, false };
/*  char Mag_Var[15]; // got to save double variable as a string! east is positive
  int Start_Page ;
  LocalTimeOffset;
  char PanelName[25];
  char APpassword[25]; 
  SOG", sizeof(config.FourWayBR));
  strlcpy(config.FourWayBL, doc["FourWayBL"] | "DEPTH", sizeof(config.FourWayBL));
  strlcpy(config.FourWayTR, doc["FourWayTR"] | "WIND", sizeof(config.FourWayTR));
  strlcpy(config.FourWayTL, doc["FourWayTL"] | "STW */
_sDisplay_Config Default_JSON = { "0.5", 4, 0, "nmeadisplay", "12345678", "SOG", "DEPTH", "WIND", "STW" };  // many display stuff set default circa 265 etc.
_sDisplay_Config Display_Config;
int Display_Page = 4;  //set last in setup(), but here for clarity?
_sWiFi_settings_Config Saved_Settings;
_sWiFi_settings_Config Current_Settings;



int MasterFont;  //global for font! Idea is to use to reset font after 'temporary' seletion of another fone
                 // to be developed ....
struct BarChart {
  int topleftx, toplefty, width, height, bordersize, value, rangemin, rangemax, visible;
  uint16_t barcolour;
};



String Fontname;
int text_height = 12;      //so we can get them if we change heights etc inside functions
int text_offset = 12;      //offset is not equal to height, as subscripts print lower than 'height'
int text_char_width = 12;  // useful for monotype? only NOT USED YET! Try gfx->getTextBounds(string, x, y, &x1, &y1, &w, &h);

//****  My displays are based on '_sButton' structures to define position, width height, borders and colours.
// int h, v, width, height, bordersize;  uint16_t BackColor, TextColor, BorderColor;

_sButton FullSize = { 10, 0, 460, 460, 0, BLUE, WHITE, BLACK };
_sButton FullSizeShadow = { 5, 10, 460, 460, 0, BLUE, WHITE, BLACK };
_sButton CurrentSettingsBox = { 0, 0, 480, 80, 2, BLUE, WHITE, BLACK };  //also used for showing the current settings

_sButton FontBox = { 0, 80, 480, 330, 5, BLUE, WHITE, BLUE };

//_sButton WindDisplay = { 0, 0, 480, 480, 0, BLUE, WHITE, BLACK };  // full screen no border

//used for single data display
// modified all to lift by 30 pixels to allow a common bottom row display (to show logs and get to settings)
_sButton StatusBox = { 0, 460, 480, 20, 0, BLACK, WHITE, BLACK };
_sButton WifiStatus = { 60, 180, 360, 120, 5, BLUE, WHITE, BLACK };  // big central box for wifi events to pop up - v3.5

_sButton BigSingleDisplay = { 0, 90, 480, 360, 5, BLUE, WHITE, BLACK };              // used for wind and graph displays
_sButton BigSingleTopRight = { 240, 0, 240, 90, 5, BLUE, WHITE, BLACK };             //  ''
_sButton BigSingleTopLeft = { 0, 0, 240, 90, 5, BLUE, WHITE, BLACK };                //  ''
_sButton TopHalfBigSingleTopRight = { 240, 0, 240, 45, 5, BLUE, WHITE, BLACK };      //  ''
_sButton BottomHalfBigSingleTopRight = { 240, 45, 240, 45, 5, BLUE, WHITE, BLACK };  //  ''
//used for nmea RMC /GPS display // was only three lines to start!
_sButton Threelines0 = { 20, 30, 440, 80, 5, BLUE, WHITE, BLACK };
_sButton Threelines1 = { 20, 130, 440, 80, 5, BLUE, WHITE, BLACK };
_sButton Threelines2 = { 20, 230, 440, 80, 5, BLUE, WHITE, BLACK };
_sButton Threelines3 = { 20, 330, 440, 80, 5, BLUE, WHITE, BLACK };
// for the quarter screens on the main page
_sButton topLeftquarter = { 0, 0, 240, 240 - 15, 5, BLUE, WHITE, BLACK };  //h  reduced by 15 to give 30 space at the bottom
_sButton bottomLeftquarter = { 0, 240 - 15, 240, 240 - 15, 5, BLUE, WHITE, BLACK };
_sButton topRightquarter = { 240, 0, 240, 240 - 15, 5, BLUE, WHITE, BLACK };
_sButton bottomRightquarter = { 240, 240 - 15, 240, 240 - 15, 5, BLUE, WHITE, BLACK };



// these were used for initial tests and for volume control - not needed for most people!! .. only used now for Range change in GPS graphic (?)
_sButton TopLeftbutton = { 0, 0, 75, 45, 5, BLUE, WHITE, BLACK };
_sButton TopRightbutton = { 405, 0, 75, 45, 5, BLUE, WHITE, BLACK };
_sButton BottomRightbutton = { 405, 405, 75, 45, 5, BLUE, WHITE, BLACK };
_sButton BottomLeftbutton = { 0, 405, 75, 45, 5, BLUE, WHITE, BLACK };

// buttons for the wifi/settings pages
_sButton TOPButton = { 20, 10, 430, 35, 5, WHITE, BLACK, BLUE };
_sButton SecondRowButton = { 20, 60, 430, 35, 5, WHITE, BLACK, BLUE };
_sButton ThirdRowButton = { 20, 100, 430, 35, 5, WHITE, BLACK, BLUE };
_sButton FourthRowButton = { 20, 140, 430, 35, 5, WHITE, BLACK, BLUE };
_sButton FifthRowButton = { 20, 180, 430, 35, 5, WHITE, BLACK, BLUE };

#define sw_width 55
//switches at line 180
_sButton Switch1 = { 20, 180, sw_width, 35, 5, WHITE, BLACK, BLUE };
_sButton Switch2 = { 100, 180, sw_width, 35, 5, WHITE, BLACK, BLUE };
_sButton Switch3 = { 180, 180, sw_width, 35, 5, WHITE, BLACK, BLUE };
_sButton Switch5 = { 260, 180, sw_width, 35, 5, WHITE, BLACK, BLUE };
_sButton Switch4 = { 345, 180, 120, 35, 5, WHITE, BLACK, BLUE };  // big one for eeprom update
//switches at line 60
_sButton Switch6 = { 20, 60, sw_width, 35, 5, WHITE, BLACK, BLACK };
_sButton Switch7 = { 100, 60, sw_width, 35, 5, WHITE, BLACK, BLACK };
_sButton Switch8 = { 180, 60, sw_width, 35, 5, WHITE, BLACK, BLACK };
_sButton Switch9 = { 260, 60, sw_width, 35, 5, WHITE, BLACK, BLACK };
_sButton Switch10 = { 340, 60, sw_width, 35, 5, WHITE, BLACK, BLACK };
_sButton Switch11 = { 420, 60, sw_width, 35, 5, WHITE, BLACK, BLACK };


_sButton Terminal = { 0, 100, 480, 330, 2, WHITE, BLACK, BLUE };
//for selections
_sButton FullTopCenter = { 80, 0, 320, 50, 5, BLUE, WHITE, BLACK };

_sButton Full0Center = { 80, 55, 320, 50, 5, BLUE, WHITE, BLACK };
_sButton Full1Center = { 80, 110, 320, 50, 5, BLUE, WHITE, BLACK };
_sButton Full2Center = { 80, 165, 320, 50, 5, BLUE, WHITE, BLACK };
_sButton Full3Center = { 80, 220, 320, 50, 5, BLUE, WHITE, BLACK };
_sButton Full4Center = { 80, 275, 320, 50, 5, BLUE, WHITE, BLACK };
_sButton Full5Center = { 80, 330, 320, 50, 5, BLUE, WHITE, BLACK };
_sButton Full6Center = { 80, 385, 320, 50, 5, BLUE, WHITE, BLACK };  // inteferes with settings box do not use!

#define On_Off ? "ON " : "OFF"  // if 1 first case else second (0 or off) same number of chars to try and helps some flashing later
#define True_False ? "true" : "false"
bool LoadVictronConfiguration(const char* filename, _sMyVictronDevices& config) {
  // Open SD file for reading
  bool fault = false;
  if (!SD.exists(filename)) {
    Serial.printf(" Json Victron file %s did not exist\n Using defaults\n", filename);
    fault = true;
  }
  File file = SD.open(filename, FILE_READ);
  if (!file) {
    Serial.println(F("**Failed to read Victron JSON file"));
    fault = true;
  }
  // Allocate a temporary JsonDocument
  char temp[15];
  JsonDocument doc;
  // Deserialize the JSON document
  DeserializationError error = deserializeJson(doc, file);
  if (error) {
    Serial.println(F("**Failed to deserialise victron JSON file"));
    fault = true;
  }
  Num_Victron_Devices = doc["Num_Devices"] | 4;
  for (int index = 0; index < Num_Victron_Devices; index++) {
    strlcpy(config.charMacAddr[index], doc["device" + String(index) + ".mac"] | "macaddress", sizeof(config.charMacAddr[index]));
    strlcpy(config.charKey[index], doc["device" + String(index) + ".key"] | "key", sizeof(config.charKey[index]));
    strlcpy(config.FileCommentName[index], doc["device" + String(index) + ".comment"] | "?name?", sizeof(config.FileCommentName[index]));
    config.VICTRON_BLE_RECORD_TYPE[index] = doc["device" + String(index) + ".VICTRON_BLE_RECORD_TYPE"] | 1;  //default solar Mppt
    config.displayH[index] = doc["device" + String(index) + ".DisplayH"];
    config.displayV[index] = doc["device" + String(index) + ".DisplayV"];
    config.displayHeight[index] = doc["device" + String(index) + ".DisplayHeight"] | 150;
    strlcpy(config.DisplayShow[index], doc["device" + String(index) + ".DisplayShow"] | "PVIA", sizeof(config.DisplayShow[index]));
  }
  // Close the file (Curiously, File's destructor doesn't close the file)
  file.close();

  return !fault;  // report success
}
void SaveVictronConfiguration(const char* filename, _sMyVictronDevices& config) {
  // USED for adding extra devices or for creating a new file if missing
  //Delete existing file, otherwise the configuration is appended to the file
  SD.remove(filename);
  char buff[15];
  // Open file for writing
  File file = SD.open(filename, FILE_WRITE);
  if (!file) {
    Serial.println(F("JSON: Victron: Failed to create SD file"));
    return;
  }
  Serial.printf(" We expect %i Victron devices", Num_Victron_Devices);
  // Allocate a temporary JsonDocument
  JsonDocument doc;
  doc[" Comment"] = " DisplayShow Options: 'P' Power 'V' Battery Volts 'I' Battery Current";
  doc[" C"] = " 'v' second Battery Volts 'i' second Battery Current  (ac charger only)";
  doc[" C1"] = "'L' Load current 'S' State of charge 'E' error codes 'T' Temperature";
  doc[" C2"] = " 'A' Aux reading(t or starter) ";


  doc[" Example"] = "for SmartShunt, IAS will display current, State of charge and Additional data( starter V or temperature)";
  doc[" Example"] = "for Battery Monitor: V will display Voltage (only)";
  doc[" note"] = "Display height is also adjustable for each 'device', and devices can be duplicated";
  doc[" note "] = "VICTRON_BLE_RECORD_TYPE:   SOLAR_CHARGER =1,  BATTERY_MONITOR = 2, AC Charger = 8";
  doc["Num_Devices"] = Num_Victron_Devices;

  // doc[" Comment1"]= "for Shunt, VIA will display Battery Volts, Current, Additional data";
  // doc[" Comment2"]= "for SOLAR, PIA will display solar Power, battery Current, Additional data";
  for (int index = 0; index < Num_Victron_Devices; index++) {
    doc["device" + String(index) + ".mac"] = config.charMacAddr[index];
    doc["device" + String(index) + ".key"] = config.charKey[index];
    doc["device" + String(index) + ".comment"] = config.FileCommentName[index];
    doc["device" + String(index) + ".VICTRON_BLE_RECORD_TYPE"] = config.VICTRON_BLE_RECORD_TYPE[index];
    doc["device" + String(index) + ".DisplayH"] = config.displayH[index];
    doc["device" + String(index) + ".DisplayV"] = config.displayV[index];
    doc["device" + String(index) + ".DisplayHeight"] = config.displayHeight[index];
    doc["device" + String(index) + ".DisplayShow"] = config.DisplayShow[index];
  }

  // Serialize JSON to file
  if (serializeJsonPretty(doc, file) == 0) {  // use 'pretty format' with line feeds
    Serial.println(F("JSON: Failed to write to Victron SD file"));
  }
  // Close the file, //but print serial as a check
  file.close();
  PrintJsonFile("Check after Saving configuration ", filename);
}
void SaveDisplayConfiguration(const char* filename, _MyColors& set) {
  // Delete existing file, otherwise the configuration is appended to the file
  SD.remove(filename);
  char buff[15];
  // Open file for writing
  File file = SD.open(filename, FILE_WRITE);
  if (!file) {
    Serial.println(F("JSON: Failed to create SD file"));
    return;
  }
  // Allocate a temporary JsonDocument
  JsonDocument doc;
  // Set the values in the JSON file.. // NOT ALL ARE read yet!!
  //modify how the display works
  doc["_comments_"] = "Colours as integers";
  doc["WHITE"] = WHITE;
  doc["BLUE"] = BLUE;
  doc["BLACK"] = BLACK;
  doc["GREEN"] = GREEN;
  doc["RED"] = RED;

  doc["TextColor"] = set.TextColor;
  doc["BackColor"] = set.BackColor;
  doc["BorderColor"] = set.BorderColor;
  doc["_comments_"] = "These sizes below are for some font tests in victron display!";
  doc["BoxH"] = set.BoxH;
  doc["BoxW"] = set.BoxW;
  doc["FontH"] = set.FontH;
  doc["FontS"] = set.FontS;
  doc["Simulate"] = set.Simulate True_False;
  doc["Debug"] = set.Debug True_False;
  doc["ShowRawDecryptedDataFor"] = set.ShowRawDecryptedDataFor;
  doc["Frame"] = set.Frame True_False;



  // Serialize JSON to file
  if (serializeJsonPretty(doc, file) == 0) {  // use 'pretty format' with line feeds
    Serial.println(F("JSON: Failed to write to SD file"));
  }
  // Close the file, but print serial as a check
  file.close();
  PrintJsonFile("Check after Saving configuration ", filename);
}
bool LoadDisplayConfiguration(const char* filename, _MyColors& set) {
  // Open SD file for reading
  File file = SD.open(filename, FILE_READ);
  if (!file) {
    Serial.println(F("**Failed to read JSON file"));
  }
  // Allocate a temporary JsonDocument
  char temp[15];
  JsonDocument doc;
  // Deserialize the JSON document
  DeserializationError error = deserializeJson(doc, file);
  if (error) {
    Serial.println(F("**Failed to deserialise JSON file"));
  }
  // gett here means we can set defaults, regardless!
  set.TextColor = doc["TextColor"] | BLACK;
  set.BackColor = doc["BackColor"] | WHITE;
  set.BorderColor = doc["BorderColor"] | BLUE;
  set.BoxW = doc["BoxW"] | 46;
  set.BoxH = doc["BoxH"] | 100;

  set.FontH = doc["FontH"] | WHITE;
  set.FontS = doc["FontS"] | WHITE;
  strlcpy(temp, doc["Simulate"] | "false", sizeof(temp));
  set.Simulate = (strcmp(temp, "false"));
  strlcpy(temp, doc["Frame"] | "false", sizeof(temp));
  set.Frame = (strcmp(temp, "false"));
  strlcpy(temp, doc["Debug"] | "false", sizeof(temp));
  set.Debug = (strcmp(temp, "false"));

  set.ShowRawDecryptedDataFor = doc["ShowRawDecryptedDataFor"] | 1;
  // Close the file (Curiously, File's destructor doesn't close the file)
  file.close();
  if (!error) { return true; }
  return false;
}

bool LoadConfiguration(const char* filename, _sDisplay_Config& config, _sWiFi_settings_Config& settings) {
  // Open SD file for reading
  if (!SD.exists(filename)) {
    Serial.printf("**JSON file %s did not exist\n Using defaults\n", filename);
    SaveConfiguration(filename, Default_JSON, Default_Settings);  //save defaults to sd file
    config = Default_JSON;
    settings = Default_Settings;
    return false;
  }
  File file = SD.open(filename, FILE_READ);
  if (!file) {
    Serial.println(F("**Failed to read JSON file"));
    return false;
  }
  // Allocate a temporary JsonDocument
  char temp[15];
  JsonDocument doc;
  // Deserialize the JSON document
  DeserializationError error = deserializeJson(doc, file);
  if (error) {
    Serial.println(F("**Failed to deserialise JSON file"));
    return false;
  }
  // Copy values (or defaults) from the JsonDocument to the config / settings
  if (doc["Start_Page"] == 0) { return false; }  //secondary backup in case the file is present (passes error) but zeroed!
  config.LocalTimeOffset = doc["LocalTimeOffset"] | 0;
  config.Start_Page = doc["Start_Page"] | 4;  // 4 is default page int - no problem...
  strlcpy(temp, doc["Mag_Var"] | "1.15", sizeof(temp));
  BoatData.Variation = atof(temp);  //  (+ = easterly) Positive: If the magnetic north is to the east of true north, the declination is positive (or easterly).

  strlcpy(config.FourWayBR, doc["FourWayBR"] | "SOG", sizeof(config.FourWayBR));
  strlcpy(config.FourWayBL, doc["FourWayBL"] | "DEPTH", sizeof(config.FourWayBL));
  strlcpy(config.FourWayTR, doc["FourWayTR"] | "WIND", sizeof(config.FourWayTR));
  strlcpy(config.FourWayTL, doc["FourWayTL"] | "STW", sizeof(config.FourWayTL));
  strlcpy(config.PanelName,                  // User selectable
          doc["PanelName"] | "NMEADISPLAY",  // <- and default in case Json is corrupt / missing !
          sizeof(config.PanelName));
  strlcpy(config.APpassword,               // User selectable
          doc["APpassword"] | "12345678",  // <- and default in case Json is corrupt / missing !
          sizeof(config.APpassword));





  // only change settings if we have read the file! else we will use the EEPROM settings
  if (!error) {
    strlcpy(settings.ssid,                 // <- destination
            doc["ssid"] | "GuestBoat",     // <- source and default in case Json is corrupt!
            sizeof(settings.ssid));        // <- destination's capacity
    strlcpy(settings.password,             // <- destination
            doc["password"] | "12345678",  // <- source and default
            sizeof(settings.password));    // <- destination's capacity
    strlcpy(settings.UDP_PORT,             // <- destination
            doc["UDP_PORT"] | "2000",      // <- source and default.
            sizeof(settings.UDP_PORT));    // <- destination's capacity

    strlcpy(temp, doc["Serial"] | "false", sizeof(temp));
    settings.Serial_on = (strcmp(temp, "false"));

    strlcpy(temp, doc["UDP"] | "true", sizeof(temp));
    settings.UDP_ON = (strcmp(temp, "false"));
    strlcpy(temp, doc["ESP"] | "false", sizeof(temp));
    settings.ESP_NOW_ON = (strcmp(temp, "false"));
    strlcpy(temp, doc["LOG"] | "false", sizeof(temp));
    settings.Log_ON = (strcmp(temp, "false"));
    strlcpy(temp, doc["NMEALOG"] | "false", sizeof(temp));
    settings.NMEA_log_ON = (strcmp(temp, "false"));
    settings.log_interval_setting = doc["LogInterval"] | 60;
  }

  strlcpy(temp, doc["BLE_enable"] | "false", sizeof(temp));
  settings.BLE_enable = (strcmp(temp, "false"));


  // Close the file (Curiously, File's destructor doesn't close the file)
  file.close();
  if (!error) { return true; }
  return false;
}


void SaveConfiguration(const char* filename, _sDisplay_Config& config, _sWiFi_settings_Config& settings) {
  // Delete existing file, otherwise the configuration is appended to the file
  SD.remove(filename);
  char buff[15];
  // Open file for writing
  File file = SD.open(filename, FILE_WRITE);
  if (!file) {
    Serial.println(F("JSON: Failed to create SD file"));
    return;
  }
  // Allocate a temporary JsonDocument
  JsonDocument doc;
  // Set the values in the JSON file.. // NOT ALL ARE read yet!!
  //modify how the display works
  doc["Start_Page"] = config.Start_Page;
  doc["LocalTimeOffset"] = config.LocalTimeOffset;
  Serial.print("save magvar:");
  Serial.printf("%5.3f", BoatData.Variation);
  snprintf(buff, sizeof(buff), "%5.3f", BoatData.Variation);
  doc["Mag_Var"] = buff;
  doc["PanelName"] = config.PanelName;
  doc["APpassword"] = config.APpassword;



  //now the settings WIFI etc..
  doc["ssid"] = settings.ssid;
  doc["password"] = settings.password;
  doc["UDP_PORT"] = settings.UDP_PORT;
  doc["Serial"] = settings.Serial_on True_False;
  doc["UDP"] = settings.UDP_ON True_False;
  doc["ESP"] = settings.ESP_NOW_ON True_False;
  doc["LogComments0"] = "LOG saves read data in file with date as name- BUT only when GPS date has been seen!";
  doc["LogComments1"] = "NMEALOG saves every message. Use NMEALOG only for debugging!";
  doc["LogComments2"] = "or the NMEALOG files will become huge";
  doc["LOG"] = settings.Log_ON True_False;
  doc["LogInterval"] = settings.log_interval_setting;
  doc["NMEALOG"] = settings.NMEA_log_ON True_False;
  doc["DisplayComment1"] = "These settings allow modification of the bottom two 'quad' displays";
  doc["DisplayComment2"] = "options are : SOG SOGGRAPH STW STWGRAPH GPS DEPTH DGRAPH DGRAPH2 ";
  doc["DisplayComment3"] = "DGRAPH  and DGRAPH2 display 10 and 30 m ranges respectively";
  doc["FourWayBR"] = config.FourWayBR;
  doc["FourWayBL"] = config.FourWayBL;
  doc["DisplayComment4"] = "Top row right can be WIND or TIME (UTC) or TIMEL (LOCAL)";
  doc["FourWayTR"] = config.FourWayTR;
  doc["FourWayTL"] = config.FourWayTL;

  doc["_comment_"] = "These settings below apply only to Victron display pages";
  doc["BLE_enable"] = settings.BLE_enable True_False;



  // Serialize JSON to file
  if (serializeJsonPretty(doc, file) == 0) {  // use 'pretty format' with line feeds
    Serial.println(F("JSON: Failed to write to SD file"));
  }
  // Close the file, but print serial as a check
  file.close();
  PrintJsonFile("Check after Saving configuration ", filename);
}


void PrintJsonFile(const char* comment, const char* filename) {
  // Open file for reading
  File file = SD.open(filename, FILE_READ);
  Serial.printf(" %s JSON FILE %s is.", comment, filename);
  if (!file) {
    Serial.println(F("Failed to read file"));
    return;
  }
  Serial.println();
  // Extract each characters by one by one
  while (file.available()) {
    Serial.print((char)file.read());
  }
  Serial.println();
  // Close the file
  file.close();
}


//****************  GRAPHICS STUFF ************************
// Draw the compass pointer at an angle in degrees
void WindArrow2(_sButton button, _sInstData Speed, _sInstData& Wind) {
  // Serial.printf(" ** DEBUG  speed %f    wind %f ",Speed.data,Wind.data);
  bool recent = (Wind.updated >= millis() - 3000);
  if (!Wind.graphed) {  //EventTiming("START");
    WindArrowSub(button, Speed, Wind);
    // EventTiming("STOP");EventTiming("WIND arrow");
  }
  if (Wind.greyed) { return; }

  if (!recent && !Wind.greyed) { WindArrowSub(button, Speed, Wind); }
}

void WindArrowSub(_sButton button, _sInstData Speed, _sInstData& wind) {
  //Serial.printf(" ** DEBUG WindArrowSub speed %f    wind %f \n",Speed.data,wind.data);
  bool recent = (wind.updated >= millis() - 3000);
  Phv center;
  int rad, outer, inner;
  static int lastfont;
  static double lastwind;
  center.h = button.h + button.width / 2;
  center.v = button.v + button.height / 2;
  rad = (button.height - (2 * button.bordersize)) / 2;  // height used as more likely to be squashed in height
  outer = (rad * 82) / 100;                             //% of full radius (at full height) (COMPASS has .83 as inner circle)
  inner = (rad * 28) / 100;                             //25% USe same settings as pointer
  DrawMeterPointer(center, lastwind, inner, outer, 2, button.BackColor, button.BackColor);
  if (wind.data != NMEA0183DoubleNA) {
    if (wind.updated >= millis() - 3000) {
      DrawMeterPointer(center, wind.data, inner, outer, 2, button.TextColor, BLACK);
    } else {
      wind.greyed = true;
      DrawMeterPointer(center, wind.data, inner, outer, 2, LIGHTGREY, LIGHTGREY);
    }
  }
  lastwind = wind.data;
  wind.graphed = true;
  lastfont = MasterFont;
  if (Speed.data != NMEA0183DoubleNA) {
    if (rad <= 130) {
      UpdateDataTwoSize(true, true, 8, 7, button, Speed, "%2.0fkt");
    } else {
      UpdateDataTwoSize(true, true, 10, 9, button, Speed, "%2.0fkt");
    }
  }

  setFont(lastfont);
}

void DrawMeterPointer(Phv center, double wind, int inner, int outer, int linewidth, uint16_t FILLCOLOUR, uint16_t LINECOLOUR) {  // WIP
  Phv P1, P2, P3, P4, P5, P6;
  P1 = translate(center, wind - linewidth, outer);
  P2 = translate(center, wind + linewidth, outer);
  P3 = translate(center, wind - (4 * linewidth), inner);
  P4 = translate(center, wind + (4 * linewidth), inner);
  P5 = translate(center, wind, inner);
  P6 = translate(center, wind, outer);
  PTriangleFill(P1, P2, P3, FILLCOLOUR);
  PTriangleFill(P2, P3, P4, FILLCOLOUR);
  Pdrawline(P5, P6, LINECOLOUR);
}

Phv translate(Phv center, double angle, int rad) {  // 'full version with full accuracy cos and sin
  Phv moved;
  moved.h = center.h + (rad * sin(angle * 0.0174533));
  moved.v = center.v - (rad * cos(angle * 0.0174533));  // v is minus as this is positive  down in gfx
  return moved;
}

void DrawCompass(_sButton button) {
  //x y are center in drawcompass
  int x, y, rad;
  x = button.h + button.width / 2;
  y = button.v + button.height / 2;
  rad = (button.height - (2 * button.bordersize)) / 2;
  int Rad1, Rad2, Rad3, Rad4, inner;
  Rad1 = rad * 0.83;  //200
  Rad2 = rad * 0.86;  //208
  Rad3 = rad * 0.91;  //220
  Rad4 = rad * 0.94;

  inner = (rad * 28) / 100;                                                            //28% USe same settings as pointer // keep border same as other boxes..
  gfx->fillRect(button.h, button.v, button.width, button.height, button.BorderColor);  // width and height are for the OVERALL box.
  gfx->fillRect(button.h + button.bordersize, button.v + button.bordersize, button.width - (2 * button.bordersize), button.height - (2 * button.bordersize), button.BackColor);
  //gfx->fillRect(x - rad, y - rad, rad * 2, rad * 2, button.BackColor);
  gfx->fillCircle(x, y, rad, button.TextColor);   //white
  gfx->fillCircle(x, y, Rad1, button.BackColor);  //bluse
  gfx->fillCircle(x, y, inner - 1, button.TextColor);
  gfx->fillCircle(x, y, inner - 5, button.BackColor);

  //rad =240 example Rad2 is 200 to 208   bar is 200 to 239 wind colours 200 to 220
  // colour segments
  gfx->fillArc(x, y, Rad3, Rad1, 270 - 45, 270, RED);
  gfx->fillArc(x, y, Rad3, Rad1, 270, 270 + 45, GREEN);
  //Mark 12 linesarks at 30 degrees
  for (int i = 0; i < (360 / 30); i++) { gfx->fillArc(x, y, rad, Rad1, i * 30, (i * 30) + 1, BLACK); }  //239 to 200
  for (int i = 0; i < (360 / 10); i++) { gfx->fillArc(x, y, rad, Rad4, i * 10, (i * 10) + 1, BLACK); }  // dots at 10 degrees
}

void ShowToplinesettings(_sWiFi_settings_Config A, String Text) {
  // int local;
  // local = MasterFont;
  // SETS MasterFont, so cannot use MasterFont directly in last line and have to save it!
  long rssiValue = WiFi.RSSI();
  gfx->setTextSize(1);
  gfx->setTextColor(CurrentSettingsBox.TextColor);
  CurrentSettingsBox.PrintLine = 0;
  // 7 is smallest Bold Font
  UpdateLinef(7, CurrentSettingsBox, "%s:SSID<%s>PWD<%s>UDPPORT<%s>", Text, A.ssid, A.password, A.UDP_PORT);
  UpdateLinef(7, CurrentSettingsBox, "IP:%i.%i.%i.%i  RSSI %i", WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3], rssiValue);
  UpdateLinef(7, CurrentSettingsBox, "Ser<%s>UDP<%s>ESP<%s>Log<%s>NMEA<%s>", A.Serial_on On_Off, A.UDP_ON On_Off, A.ESP_NOW_ON On_Off, A.Log_ON On_Off, A.NMEA_log_ON On_Off);
  // UpdateLinef(7,CurrentSettingsBox, "Logger settings Log<%s>NMEA<%s>",A.Serial_on On_Off, A.UDP_ON On_Off, A.ESP_NOW_ON On_Off,A.NMEA_log_ON On_Off);
}
void ShowToplinesettings(String Text) {
  ShowToplinesettings(Current_Settings, Text);
}

void ShowGPSinBox(int font, _sButton button) {
  static double lastTime;
  //Serial.printf("In ShowGPSinBox  %i\n",int(BoatData.GPSTime));
  if ((BoatData.GPSTime != NMEA0183DoubleNA) && (BoatData.GPSTime != lastTime)) {
    lastTime = BoatData.GPSTime;
    GFXBorderBoxPrintf(button, "");
    AddTitleInsideBox(9, 2, button, " GPS");
    button.PrintLine = 0;
    if (BoatData.SatsInView != NMEA0183DoubleNA) { UpdateLinef(font, button, "Satellites in view %.0f ", BoatData.SatsInView); }
    if (BoatData.GPSTime != NMEA0183DoubleNA) {
      //UpdateLinef(font, button, "");
      UpdateLinef(font, button, "Date: %06i ", int(BoatData.GPSDate));
      //UpdateLinef(font, button, "");
      UpdateLinef(font, button, "TIME: %02i:%02i:%02i",
                  int(BoatData.GPSTime) / 3600, (int(BoatData.GPSTime) % 3600) / 60, (int(BoatData.GPSTime) % 3600) % 60);
    }
    if (BoatData.Latitude.data != NMEA0183DoubleNA) {
      UpdateLinef(font, button, "LAT");
      UpdateLinef(font, button, "%s", LattoString(BoatData.Latitude.data));
      UpdateLinef(font, button, "LON");
      UpdateLinef(font, button, "%s", LongtoString(BoatData.Longitude.data));
      UpdateLinef(font, button, "");
    }
  }
}

//***************************   DISPLAY .. The main place where the pages are described ****************
void Display(int page) {
  Display(false, page);
}

void showPicture(const char* name) {
  jpegDraw(name, jpegDrawCallback, true /* useBigEndian */,
           0 /* x */, 0 /* y */, gfx->width() /* widthLimit */, gfx->height() /* heightLimit */);
}
void showPictureFrame(_sButton& button, const char* name) {
  if (!SD.exists(name)) { return; }
  jpegDraw(name, jpegDrawCallback, true /* useBigEndian */,
           button.h /* x */, button.v /* y */, button.width /* widthLimit */, button.height /* heightLimit */);
  gfx->fillRect(button.h + button.bordersize, button.v + button.bordersize,
                button.width - (2 * button.bordersize), button.height - (2 * button.bordersize), button.BackColor);
}

void Display(bool reset, int page) {  // setups for alternate pages to be selected by page.
  static unsigned long flashinterval;
  static bool flash;
  static double startposlat, startposlon;
  double LatD, LongD;  //deltas
  double wind_gnd;
  int magnification, h, v;

  static int LastPageselected;
  static bool DataChanged;
  static int wifissidpointer;
  // some local variables for tests;
  //char blank[] = "null";
  static int SwipeTestLR, SwipeTestUD, volume;
  static bool RunSetup;
  static unsigned int slowdown, timer2;
  //static float wind, SOG, Depth;
  float temp;
  static _sInstData LocalCopy;  // needed only where two digital displays wanted for the same data variable.
  static int fontlocal;
  static int FileIndex, Playing;  // static to hold after selection and before pressing play!
  static int V_offset;            // used in the audio file selection to sort print area
  char Tempchar[30];
  //String tempstring;
  // int FS = 1;  // for font size test
  // int tempint;
  if (page != LastPageselected) {
    WIFIGFXBoxdisplaystarted = false;  // will have reset the screen, so turn off the auto wifibox blanking if there was a wifiiterrupt box
                                       // this (above) saves a timed screen refresh that can clear keyboard stuff
    RunSetup = true;
  }
  if (reset) {
    WIFIGFXBoxdisplaystarted = false;
    RunSetup = true;
  }
  //generic setup stuff for ALL pages
  if (RunSetup) {
    gfx->fillScreen(BLUE);
    gfx->setTextColor(WHITE);
    setFont(3);
    GFXBorderBoxPrintf(StatusBox, "");  // common to all pages
  }
  if ((millis() >= flashinterval)) {
    flashinterval = millis() + 1000;
    StatusBox.PrintLine = 0;  // always start / only use / the top line 0  of this box
    UpdateLinef(3, StatusBox, "%s page:%i  Log Status %s NMEA %s  ", Display_Config.PanelName, Display_Page,
                Current_Settings.Log_ON On_Off, Current_Settings.NMEA_log_ON On_Off);
    if (Current_Settings.Log_ON || Current_Settings.NMEA_log_ON) {
      flash = !flash;
      if (!flash) {
        UpdateLinef(3, StatusBox, "%s page:%i  Log Status     NMEA     ", Display_Config.PanelName, Display_Page);
      }
    }
  }
  // add any other generic stuff here
  if (CheckButton(StatusBox)) { Display_Page = 0; }  // go to settings
  // Now specific stuff for each page

  switch (page) {  // just show the logos on the sd card top page
    case -200:
      if (RunSetup) {
        showPicture("/logo.jpg");
        // jpegDraw("/logo.jpg", jpegDrawCallback, true /* useBigEndian */,
        //          0 /* x */, 0 /* y */, gfx->width() /* widthLimit */, gfx->height() /* heightLimit */);
        GFXBorderBoxPrintf(Full0Center, "Jpg tests -Return to Menu-");
        GFXBorderBoxPrintf(Full1Center, "logo.jpg");
        GFXBorderBoxPrintf(Full2Center, "logo1.jpg");
        GFXBorderBoxPrintf(Full3Center, "logo2.jpg");
        GFXBorderBoxPrintf(Full4Center, "logo4.jpg");
        GFXBorderBoxPrintf(Full5Center, "logo4.jpg");
      }

      if (CheckButton(Full0Center)) { Display_Page = 0; }
      if (CheckButton(Full1Center)) {
        jpegDraw("/logo.jpg", jpegDrawCallback, true /* useBigEndian */,
                 0 /* x */, 0 /* y */, gfx->width() /* widthLimit */, gfx->height() /* heightLimit */);
        GFXBorderBoxPrintf(Full0Center, "logo");
      }
      if (CheckButton(Full2Center)) {
        jpegDraw("/logo1.jpg", jpegDrawCallback, true /* useBigEndian */,
                 0 /* x */, 0 /* y */, gfx->width() /* widthLimit */, gfx->height() /* heightLimit */);
        GFXBorderBoxPrintf(Full0Center, "logo1");
      }
      if (CheckButton(Full3Center)) {
        jpegDraw("/logo2.jpg", jpegDrawCallback, false /* useBigEndian */,
                 0 /* x */, 0 /* y */, gfx->width() /* widthLimit */, gfx->height() /* heightLimit */);
        GFXBorderBoxPrintf(Full0Center, "logo2");
      }
      if (CheckButton(Full4Center)) {
        jpegDraw("/logo4.jpg", jpegDrawCallback, true /* useBigEndian */,
                 0 /* x */, 0 /* y */, gfx->width() /* widthLimit */, gfx->height() /* heightLimit */);
        GFXBorderBoxPrintf(Full0Center, "logo4");
      }
      if (CheckButton(Full5Center)) {
        jpegDraw("/logo4.jpg", jpegDrawCallback, true /* useBigEndian */,
                 0 /* x */, 0 /* y */, gfx->width() /* widthLimit */, gfx->height() /* heightLimit */);
        GFXBorderBoxPrintf(Full0Center, "logo4");
      }

      break;

    case -99:  //a test for Screen Colours / fonts
      if (RunSetup) {
        gfx->fillScreen(BLACK);
        gfx->setTextColor(WHITE);
        fontlocal = 0;
        SwipeTestLR = 0;
        SwipeTestUD = 0;
        setFont(fontlocal);
        GFXBorderBoxPrintf(CurrentSettingsBox, "-TEST Colours- ");
      }

      if (millis() >= slowdown + 10000) {
        slowdown = millis();
        gfx->fillScreen(BLACK);
        fontlocal = fontlocal + 1;
        if (fontlocal > 10) { fontlocal = 0; }
        GFXBorderBoxPrintf(CurrentSettingsBox, "Font size %i", fontlocal);
      }



      break;
    case -87:  // page for graphic display of Vicron data
      if (RunSetup) {
        jpegDraw("/vicback.jpg", jpegDrawCallback, true /* useBigEndian */,
                 0 /* x */, 0 /* y */, gfx->width() /* widthLimit */, gfx->height() /* heightLimit */);
        Serial.println("redrawing background");
      }

      // all graphics done in VICTRONBLE
      if (CheckButton(FullTopCenter)) { Display_Page = -86; }
      break;
    case -86:                                              // page for text display of Vicron data
      if (RunSetup) { GFXBorderBoxPrintf(Terminal, ""); }  // only for setup, not changed data
      if (RunSetup || DataChanged) {
        setFont(3);  // different from most pages, displays in terminal from see ~line 2145
        GFXBorderBoxPrintf(FullTopCenter, "Return to VICTRON graphic display");
        if (!Terminal.debugpause) {
          AddTitleBorderBox(0, Terminal, "-running-");
        } else {
          AddTitleBorderBox(0, Terminal, "-paused-");
        }
        DataChanged = false;
      }
      // if (millis() > slowdown + 500) {
      //   slowdown = millis();
      // }
      if (CheckButton(FullTopCenter)) { Display_Page = -87; }
      if (CheckButton(Terminal)) {
        Terminal.debugpause = !Terminal.debugpause;
        DataChanged = true;  // for more immediate visual response to touch!
        if (!Terminal.debugpause) {
          AddTitleBorderBox(0, Terminal, "-running-");
        } else {
          AddTitleBorderBox(0, Terminal, "-paused-");
        }
      }

      break;






    case -20:  // Experimental / extra stuff
      if (RunSetup || DataChanged) {
        ShowToplinesettings("Now");
        setFont(3);
        setFont(3);
        GFXBorderBoxPrintf(Full0Center, "-Test JPegs-");
        GFXBorderBoxPrintf(Full1Center, "Check SD /Audio");
        GFXBorderBoxPrintf(Full2Center, "Check Fonts");
        GFXBorderBoxPrintf(Full3Center, "VICTRON devices");
        // GFXBorderBoxPrintf(Full3Center, "See NMEA");

        GFXBorderBoxPrintf(Full5Center, "Main Menu");
      }
      if (millis() > slowdown + 500) {
        slowdown = millis();
      }
      if (CheckButton(Full0Center)) { Display_Page = -200; }
      if (CheckButton(Full1Center)) { Display_Page = -9; }
      if (CheckButton(Full2Center)) { Display_Page = -10; }
      if (CheckButton(Full3Center)) { Display_Page = -87; }  // V for Victron go straight to graphic display
      //   if (CheckButton(Full4Center)) { Display_Page = -10; }
      if (CheckButton(Full5Center)) { Display_Page = 0; }
      break;

    case -21:                                              // Secondary "Log and debug "
      if (RunSetup) { GFXBorderBoxPrintf(Terminal, ""); }  // only for setup, not changed data
      if (RunSetup || DataChanged) {
        setFont(3);
        GFXBorderBoxPrintf(FullTopCenter, "Instrument Data / NMEA Logging");
        GFXBorderBoxPrintf(Switch6, Current_Settings.Log_ON On_Off);
        AddTitleBorderBox(0, Switch6, "Inst LOG");
        GFXBorderBoxPrintf(Switch7, Current_Settings.NMEA_log_ON On_Off);
        AddTitleBorderBox(0, Switch7, "NMEA LOG");
        GFXBorderBoxPrintf(Switch9, Current_Settings.UDP_ON On_Off);
        AddTitleBorderBox(0, Switch9, "UDP");
        GFXBorderBoxPrintf(Switch10, Current_Settings.ESP_NOW_ON On_Off);
        AddTitleBorderBox(0, Switch10, "ESP-Now");

        if (!Terminal.debugpause) {
          AddTitleBorderBox(0, Terminal, "TERMINAL");
        } else {
          AddTitleBorderBox(0, Terminal, "-Paused-");
        }
        DataChanged = false;
      }
      // if (millis() > slowdown + 500) {
      //   slowdown = millis();
      // }
      if (CheckButton(FullTopCenter)) { Display_Page = 0; }
      if (CheckButton(Terminal)) {
        Terminal.debugpause = !Terminal.debugpause;
        DataChanged = true;
        if (!Terminal.debugpause) {
          AddTitleBorderBox(0, Terminal, "-running-");
        } else {
          AddTitleBorderBox(0, Terminal, "-paused-");
        }
      }
      if (CheckButton(Switch6)) {
        Current_Settings.Log_ON = !Current_Settings.Log_ON;
        if (Current_Settings.Log_ON) { StartInstlogfile(); }
        DataChanged = true;
      };

      if (CheckButton(Switch7)) {
        Current_Settings.NMEA_log_ON = !Current_Settings.NMEA_log_ON;
        if (Current_Settings.NMEA_log_ON) { StartNMEAlogfile(); }
        DataChanged = true;
      };

      if (CheckButton(Switch9)) {
        Current_Settings.UDP_ON = !Current_Settings.UDP_ON;
        DataChanged = true;
      };
      if (CheckButton(Switch10)) {
        Current_Settings.ESP_NOW_ON = !Current_Settings.ESP_NOW_ON;
        DataChanged = true;
      };



      break;

    case -10:  // a test page for fonts
      if (RunSetup || DataChanged) {
        gfx->fillScreen(BLUE);
        setFont(3);
        // GFXBorderBoxPrintf(Full0Center, "-Font test -");
        GFXBorderBoxPrintf(BottomLeftbutton, "Smaller");
        GFXBorderBoxPrintf(BottomRightbutton, "Larger");
      }
      // if (millis() > slowdown + 5000) {
      //   slowdown = millis();
      //   //gfx->fillScreen(BLUE);
      //   // fontlocal = fontlocal + 1;
      //   // if (fontlocal > 15) { fontlocal = 0; } // just use the buttons to change!
      //   temp = 12.3;
      //   setFont(fontlocal);
      // }
      if (millis() > timer2 + 500) {
        timer2 = millis();
        temp = random(-9000, 9000);
        temp = temp / 1000;
        setFont(fontlocal);
        Fontname.toCharArray(Tempchar, 30, 0);
        int FontHt;
        setFont(fontlocal);
        FontHt = text_height;
        setFont(3);
        GFXBorderBoxPrintf(CurrentSettingsBox, "FONT:%i name%s height<%i>", fontlocal, Tempchar, FontHt);
        setFont(fontlocal);
        GFXBorderBoxPrintf(FontBox, "Test %4.2f", temp);
        DataChanged = false;
      }
      if (CheckButton(Full0Center)) { Display_Page = 0; }

      if (CheckButton(BottomLeftbutton)) {
        fontlocal = fontlocal - 1;
        DataChanged = true;
      }
      if (CheckButton(BottomRightbutton)) {
        fontlocal = fontlocal + 1;
        DataChanged = true;
      }
      break;

    case -9:  // Play with the audio ..NOTE  Needs the resistors resoldered to connect to the audio chip on the Guitron (mains relay type) version!!
      if (RunSetup || DataChanged) {
        setFont(4);
        gfx->setTextColor(WHITE, BLUE);
        if (volume > 21) { volume = 21; };
        GFXBorderBoxPrintf(TOPButton, "Main Menu");
        GFXBorderBoxPrintf(SecondRowButton, "PLAY Audio vol %i", volume);
        GFXBorderBoxPrintf(BottomLeftbutton, "vol-");
        GFXBorderBoxPrintf(BottomRightbutton, "vol+");
        //gfx->setCursor(0, 80);

        int file_num;
        // = get_music_list(SD, "/", 0, file_list);  // see https://github.com/VolosR/MakePythonLCDMP3/blob/main/MakePythonLCDMP3.ino#L101
        file_num = get_music_list(SD, "/music", 0);                                                             // Char array  based version of  https://github.com/VolosR/MakePythonLCDMP3/blob/main/MakePythonLCDMP3.ino#L101
        V_offset = text_height + (ThirdRowButton.v + ThirdRowButton.height + (3 * ThirdRowButton.bordersize));  // start below the third button
        int localy = 0;
        for (int i = 0; i < file_num; i++) {
          gfx->setCursor(0, V_offset + localy);
          gfx->print("   ");
          gfx->println(File_List[i]);
          Serial.println(File_List[i]);
          localy = localy + text_height;
        }
        GFXBorderBoxPrintf(ThirdRowButton, "Found %i files", file_num);
        DataChanged = false;
      }

      if (millis() > slowdown + 1000) {
        slowdown = millis();
        //DataChanged = false;
        //periodic updates.. other stuff?
        if (audio.isRunning()) {
          GFXBorderBoxPrintf(SecondRowButton, "Playing:%s", File_List[Playing]);
        } else {
          GFXBorderBoxPrintf(SecondRowButton, "PLAY Audio vol %i", volume);
        }
      }
      if ((ts.isTouched) && (ts.points[0].y >= V_offset) && (ts.points[0].y <= BottomLeftbutton.v)) {  // nb check on location on screen or it will get reset when you press one of the boxes
        //TouchCrosshair(1); // to help debugging only!
        FileIndex = ((ts.points[0].y - V_offset) / text_height) - 1;
        // Serial.printf(" pointing at %i %s\n", FileIndex, File_List[FileIndex]);
        GFXBorderBoxPrintf(ThirdRowButton, "%s", File_List[FileIndex]);
      }

      if (CheckButton(SecondRowButton)) {
        Playing = FileIndex;
        volume = 8;
        open_new_song("/music/", File_List[Playing]);
        GFXBorderBoxPrintf(SecondRowButton, "Playing:%s", File_List[Playing]);
      }

      if (CheckButton(TOPButton)) { Display_Page = 0; }
      if (CheckButton(TopLeftbutton)) { Display_Page = 0; }
      if (CheckButton(TopRightbutton)) { Display_Page = 0; }
      if (CheckButton(BottomRightbutton)) {
        volume = volume + 1;
        if (volume > 21) { volume = 21; };
        audio.setVolume(volume);
        DataChanged = true;
      }
      if (CheckButton(BottomLeftbutton)) {
        volume = volume - 1;
        if (volume < 1) { volume = 0; };
        audio.setVolume(volume);
        DataChanged = true;
      }
      break;

    case -5:  ///Wifiscan

      if (RunSetup || DataChanged) {
        setFont(4);
        if (IsConnected) {
          GFXBorderBoxPrintf(TOPButton, "Connected<%s>", Current_Settings.ssid);
        } else {
          GFXBorderBoxPrintf(TOPButton, "NOT FOUND:%s", Current_Settings.ssid);
        }
        GFXBorderBoxPrintf(TopRightbutton, "Scan");
        GFXBorderBoxPrintf(SecondRowButton, " Saved results");
        gfx->setCursor(0, 140);  //(location of terminal. make better later!)

        if (NetworksFound <= 0) {  // note scan error can give negative number
          NetworksFound = 0;
          GFXBorderBoxPrintf(SecondRowButton, " Use keyboard ");
        } else {
          GFXBorderBoxPrintf(SecondRowButton, " %i Networks Found", NetworksFound);
          gfx->fillRect(0, 200, 480, 280, BLUE);  // clear the place wherethe wifi wil be printed
          for (int i = 0; i < NetworksFound; ++i) {
            gfx->setCursor(0, 200 + ((i + 1) * text_height));
            // Print SSID and RSSI for each network found
            gfx->print(i + 1);
            gfx->print(": ");
            //if (WiFi.SSID(i).length() > 20) { gfx->print("..(toolong).."); }
            gfx->print(WiFi.SSID(i).substring(0, 20));
            if (WiFi.SSID(i).length() > 20) { gfx->print(".."); }
            Serial.print(WiFi.SSID(i));
            Serial.println(WiFi.channel(i));
            gfx->print(" (");
            gfx->print(WiFi.RSSI(i));
            gfx->print(")");
            gfx->print(" ch<");
            gfx->print(WiFi.channel(i));
            gfx->println(">");


            //    gfx->println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " " : "*");
            delay(10);
          }
        }
        DataChanged = false;
      }

      if (millis() > slowdown + 1000) {
        slowdown = millis();
        //other stuff?
      }

      if ((ts.isTouched) && (ts.points[0].y >= 200)) {  // nb check on location on screen or it will get reset when you press one of the boxes
        //TouchCrosshair(1);
        wifissidpointer = ((ts.points[0].y - 200) / text_height) - 1;
        int str_len = WiFi.SSID(wifissidpointer).length() + 1;
        char result[str_len];
        //  Serial.printf(" touched at %i  equates to %i ? %s ", ts.points[0].y, wifissidpointer, WiFi.SSID(wifissidpointer));
        //  Serial.printf("  result str_len%i   sizeof settings.ssid%i \n", str_len, sizeof(Current_Settings.ssid));
        if (str_len <= sizeof(Current_Settings.ssid)) {                                       // check small enough for our ssid register array!
          WiFi.SSID(wifissidpointer).toCharArray(result, sizeof(Current_Settings.ssid) - 1);  // I like to keep a spare space!
          if (str_len == 1) {
            GFXBorderBoxPrintf(SecondRowButton, "Set via Keyboard?");
          } else {
            GFXBorderBoxPrintf(SecondRowButton, "Select<%s>?", result);
          }
        } else {
          GFXBorderBoxPrintf(SecondRowButton, "ssid too long ");
        }
      }
      if (CheckButton(TopRightbutton)) {
        GFXBorderBoxPrintf(SecondRowButton, " Starting WiFi re-SCAN / reconnect ");
        AttemptingConnect = false;  // so that Scan can do a full scan..
        ScanAndConnect(false);
        DataChanged = true;
      }  // do the scan again
      if (CheckButton(SecondRowButton)) {
        // Serial.printf(" * Debug wifissidpointer=%i \n",wifissidpointer);
        if ((NetworksFound >= 1) && (wifissidpointer <= NetworksFound)) {
          WiFi.SSID(wifissidpointer).toCharArray(Current_Settings.ssid, sizeof(Current_Settings.ssid) - 1);
          Serial.printf("Update ssid to <%s> \n", Current_Settings.ssid);
          Display_Page = -1;
        } else {
          Serial.println("Update ssid via keyboard");
          Display_Page = -2;
        }
      }
      if (CheckButton(TOPButton)) { Display_Page = -1; }
      break;

    case -4:  // Keyboard setting of UDP port - note keyboard (2) numbers start
      if (RunSetup) {
        setFont(4);
        GFXBorderBoxPrintf(TOPButton, "Current <%s>", Current_Settings.UDP_PORT);
        GFXBorderBoxPrintf(Full0Center, "Set UDP PORT");
        keyboard(-1);  //reset
        keyboard(2);
        //Use_Keyboard(blank, sizeof(blank));
        Use_Keyboard(Current_Settings.UDP_PORT, sizeof(Current_Settings.UDP_PORT));
      }
      Use_Keyboard(Current_Settings.UDP_PORT, sizeof(Current_Settings.UDP_PORT));
      if (CheckButton(TOPButton)) { Display_Page = -1; }
      break;

    case -3:  // keyboard setting of Password
      if (RunSetup) {
        setFont(4);
        GFXBorderBoxPrintf(TOPButton, "Current <%s>", Current_Settings.password);
        GFXBorderBoxPrintf(Full0Center, "Set Password");
        keyboard(-1);  //reset
        Use_Keyboard(Current_Settings.password, sizeof(Current_Settings.password));
        keyboard(1);
      }
      Use_Keyboard(Current_Settings.password, sizeof(Current_Settings.password));
      if (CheckButton(Full0Center)) { Display_Page = -1; }
      if (CheckButton(TOPButton)) { Display_Page = -1; }

      break;

    case -2:  //Keyboard set of SSID
      if (RunSetup) {
        setFont(4);
        GFXBorderBoxPrintf(Full0Center, "Set SSID");
        GFXBorderBoxPrintf(TopRightbutton, "Scan");
        AddTitleBorderBox(0, TopRightbutton, "WiFi");
        keyboard(-1);  //reset
        Use_Keyboard(Current_Settings.ssid, sizeof(Current_Settings.ssid));
        keyboard(1);
      }

      Use_Keyboard(Current_Settings.ssid, sizeof(Current_Settings.ssid));
      if (CheckButton(Full0Center)) { Display_Page = -1; }
      if (CheckButton(TopRightbutton)) { Display_Page = -5; }
      break;

    case -1:  // this is the WIFI settings page
      if (RunSetup || DataChanged) {
        gfx->fillScreen(BLACK);
        gfx->setTextColor(BLACK);
        gfx->setTextSize(1);
        EEPROM_READ();
        ShowToplinesettings(Saved_Settings, "EEPROM");
        setFont(4);
        GFXBorderBoxPrintf(SecondRowButton, "SSID <%s>", Current_Settings.ssid);
        if (IsConnected) {
          AddTitleBorderBox(0, SecondRowButton, "Current Setting <CONNECTED>");
        } else {
          AddTitleBorderBox(0, SecondRowButton, "Current Setting <NOT CONNECTED>");
        }
        GFXBorderBoxPrintf(ThirdRowButton, "Password <%s>", Current_Settings.password);
        AddTitleBorderBox(0, ThirdRowButton, "Current Setting");
        GFXBorderBoxPrintf(FourthRowButton, "UDP Port <%s>", Current_Settings.UDP_PORT);
        AddTitleBorderBox(0, FourthRowButton, "Current Setting");
        GFXBorderBoxPrintf(Switch1, Current_Settings.Serial_on On_Off);  //A.Serial_on On_Off,  A.UDP_ON On_Off, A.ESP_NOW_ON On_Off
        AddTitleBorderBox(0, Switch1, "Serial");
        GFXBorderBoxPrintf(Switch2, Current_Settings.UDP_ON On_Off);
        AddTitleBorderBox(0, Switch2, "UDP");
        GFXBorderBoxPrintf(Switch3, Current_Settings.ESP_NOW_ON On_Off);
        AddTitleBorderBox(0, Switch3, "ESP-Now");
        Serial.printf(" Compare Saved and Current <%s> \n", CompStruct(Saved_Settings, Current_Settings) ? "-same-" : "UPDATE");
        GFXBorderBoxPrintf(Switch4, CompStruct(Saved_Settings, Current_Settings) ? "-same-" : "UPDATE");
        AddTitleBorderBox(0, Switch4, "EEPROM");
        GFXBorderBoxPrintf(Full5Center, "Logger and Debug");
        setFont(3);
        DataChanged = false;
      }
      if (millis() > slowdown + 1000) {
        slowdown = millis();
      }
      //runsetup to repopulate the text in the boxes!
      if (CheckButton(Switch1)) {
        Current_Settings.Serial_on = !Current_Settings.Serial_on;
        DataChanged = true;
      };
      if (CheckButton(Switch2)) {
        Current_Settings.UDP_ON = !Current_Settings.UDP_ON;
        DataChanged = true;
      };
      if (CheckButton(Switch3)) {
        Current_Settings.ESP_NOW_ON = !Current_Settings.ESP_NOW_ON;
        DataChanged = true;
      };

      if (CheckButton(Switch4)) {
        EEPROM_WRITE(Display_Config, Current_Settings);
        delay(50);
        // Display_Page = 0;
        DataChanged = true;
      };

      if (CheckButton(TOPButton)) { Display_Page = 0; }
      //if (CheckButton(Full0Center)) { Display_Page = 0; }
      if (CheckButton(SecondRowButton)) { Display_Page = -5; };
      if (CheckButton(ThirdRowButton)) { Display_Page = -3; };
      if (CheckButton(FourthRowButton)) { Display_Page = -4; };
      if (CheckButton(Full5Center)) { Display_Page = -21; };
      break;

    case 0:  // main settings
      if (RunSetup) {
        ShowToplinesettings("Now");
        setFont(4);
        GFXBorderBoxPrintf(Full0Center, "-Experimental-");
        GFXBorderBoxPrintf(Full1Center, "WIFI Settings");
        GFXBorderBoxPrintf(Full2Center, "NMEA DISPLAY");
        GFXBorderBoxPrintf(Full3Center, "Debug + LOG");
        GFXBorderBoxPrintf(Full4Center, "GPS Display");
        GFXBorderBoxPrintf(Full5Center, "Victron Display");
        GFXBorderBoxPrintf(Full6Center, "Save / Reset ");
      }
      if (millis() > slowdown + 500) {
        slowdown = millis();
      }
      if (CheckButton(Full0Center)) { Display_Page = -20; }
      if (CheckButton(Full1Center)) { Display_Page = -1; }
      if (CheckButton(Full2Center)) { Display_Page = 4; }
      if (CheckButton(Full3Center)) { Display_Page = -21; }
      if (CheckButton(Full4Center)) { Display_Page = 9; }
      if (CheckButton(Full5Center)) { Display_Page = -87; }
      if (CheckButton(Full6Center)) {
        //Display_Page = 4;
        EEPROM_WRITE(Display_Config, Current_Settings);
        delay(50);
        WiFi.disconnect();
        ESP.restart();
      }
      break;

    case 4:  // Quad display
      if (RunSetup) {
        setFont(10);
        gfx->fillScreen(BLACK);
        // DrawCompass(360, 120, 120);
        if (String(Display_Config.FourWayTR) == "WIND") {
          DrawCompass(topRightquarter);
          AddTitleInsideBox(8, 3, topRightquarter, "WIND APP ");
        }
        GFXBorderBoxPrintf(topLeftquarter, "");
        AddTitleInsideBox(9, 3, topLeftquarter, "STW ");
        AddTitleInsideBox(9, 2, topLeftquarter, " Kts");  //font,position
        setFont(10);
      }
      if (millis() > slowdown + 1000) {
        slowdown = millis();  //only make/update copies every second!  else undisplayed copies will be redrawn!
        // could have more complex that accounts for displayed already?
        setFont(12);
        if (String(Display_Config.FourWayTR) == "TIME") {
          GFXBorderBoxPrintf(topRightquarter, "%02i:%02i",
                             int(BoatData.GPSTime) / 3600, (int(BoatData.GPSTime) % 3600) / 60);
          AddTitleInsideBox(9, 3, topRightquarter, "UTC ");
        }
        if (String(Display_Config.FourWayTR) == "TIMEL") {
          GFXBorderBoxPrintf(topRightquarter, "%02i:%02i",
                             int(BoatData.LOCTime) / 3600, (int(BoatData.LOCTime) % 3600) / 60);
          AddTitleInsideBox(9, 3, topRightquarter, "LOCAL ");
        }
        setFont(10);
      }

      UpdateDataTwoSize(true, true, 13, 11, topLeftquarter, BoatData.STW, "%.1f");


      if (String(Display_Config.FourWayTR) == "WIND") { WindArrow2(topRightquarter, BoatData.WindSpeedK, BoatData.WindAngleApp); }


      //seeing if JSON setting of (bottom two sides of) quad is useful.. TROUBLE with two scrollGraphss so there is now extra 'instances' settings allowing two to run simultaneously!! ?

      if (String(Display_Config.FourWayBL) == "DEPTH") { UpdateDataTwoSize(RunSetup, "DEPTH", " M", true, true, 13, 11, bottomLeftquarter, BoatData.WaterDepth, "%.1f"); }
      if (String(Display_Config.FourWayBL) == "SOG") { UpdateDataTwoSize(RunSetup, "SOG", " Kts", true, true, 13, 11, bottomLeftquarter, BoatData.SOG, "%.1f"); }
      if (String(Display_Config.FourWayBL) == "STW") { UpdateDataTwoSize(RunSetup, "STW", " Kts", true, true, 13, 11, bottomLeftquarter, BoatData.STW, "%.1f"); }

      if (String(Display_Config.FourWayBL) == "GPS") { ShowGPSinBox(9, bottomLeftquarter); }

      if (String(Display_Config.FourWayBL) == "DGRAPH") { SCROLLGraph(RunSetup, 0, 1, true, bottomLeftquarter, BoatData.WaterDepth, 10, 0, 8, "Fathmometer 10m ", "m"); }
      if (String(Display_Config.FourWayBL) == "DGRAPH2") { SCROLLGraph(RunSetup, 0, 1, true, bottomLeftquarter, BoatData.WaterDepth, 50, 0, 8, "Fathmometer 50m ", "m"); }
      if (String(Display_Config.FourWayBL) == "STWGRAPH") { SCROLLGraph(RunSetup, 0, 1, true, bottomLeftquarter, BoatData.STW, 0, 10, 8, "STW ", "kts"); }
      if (String(Display_Config.FourWayBL) == "SOGGRAPH") { SCROLLGraph(RunSetup, 0, 1, true, bottomLeftquarter, BoatData.SOG, 0, 10, 8, "SOG ", "kts"); }

      // note use of SCROLLGraph2
      if (String(Display_Config.FourWayBR) == "DEPTH") { UpdateDataTwoSize(RunSetup, "DEPTH", " M", true, true, 13, 11, bottomRightquarter, BoatData.WaterDepth, "%.1f"); }
      if (String(Display_Config.FourWayBR) == "SOG") { UpdateDataTwoSize(RunSetup, "SOG", " Kts", true, true, 13, 11, bottomRightquarter, BoatData.SOG, "%.1f"); }
      if (String(Display_Config.FourWayBR) == "STW") { UpdateDataTwoSize(RunSetup, "STW", " Kts", true, true, 13, 11, bottomRightquarter, BoatData.STW, "%.1f"); }

      if (String(Display_Config.FourWayBR) == "GPS") { ShowGPSinBox(9, bottomRightquarter); }

      if (String(Display_Config.FourWayBR) == "DGRAPH") { SCROLLGraph(RunSetup, 1, 1, true, bottomRightquarter, BoatData.WaterDepth, 10, 0, 8, "Fathmometer 10m ", "m"); }
      if (String(Display_Config.FourWayBR) == "DGRAPH2") { SCROLLGraph(RunSetup, 1, 1, true, bottomRightquarter, BoatData.WaterDepth, 50, 0, 8, "Fathmometer 50m ", "m"); }
      if (String(Display_Config.FourWayBR) == "SOGGRAPH") { SCROLLGraph(RunSetup, 1, 1, true, bottomRightquarter, BoatData.SOG, 0, 10, 8, "SOG ", "kts"); }
      if (String(Display_Config.FourWayBR) == "STWGRAPH") { SCROLLGraph(RunSetup, 1, 1, true, bottomRightquarter, BoatData.STW, 0, 10, 8, "STW ", "kts"); }


      if (CheckButton(topLeftquarter)) { Display_Page = 6; }     //stw
      if (CheckButton(bottomLeftquarter)) { Display_Page = 7; }  //depth
      if (CheckButton(topRightquarter)) { Display_Page = 5; }    // Wind
      if (CheckButton(bottomRightquarter)) {
        if (String(Display_Config.FourWayBR) == "GPS") {
          Display_Page = 9;
        } else {
          Display_Page = 8;
        }  //SOG
      }

      break;

    case 5:  // wind instrument
      if (RunSetup) {
        setFont(10);
        GFXBorderBoxPrintf(BigSingleDisplay, "");
        GFXBorderBoxPrintf(BigSingleTopRight, "");
        AddTitleInsideBox(8, 2, BigSingleTopRight, " deg");
        DrawCompass(BigSingleDisplay);
        AddTitleInsideBox(8, 3, BigSingleDisplay, "WIND Apparent ");
      }
      if (millis() > slowdown + 500) {
        slowdown = millis();
      }

      WindArrow2(BigSingleDisplay, BoatData.WindSpeedK, BoatData.WindAngleApp);
      UpdateDataTwoSize(true, true, 12, 10, BigSingleTopRight, BoatData.WindAngleApp, "%.1f");
      if (CheckButton(BigSingleDisplay)) { Display_Page = 15; }
      if (CheckButton(topLeftquarter)) { Display_Page = 4; }
      break;

    case 6:  //STW Speed Through WATER GRAPH
      if (RunSetup) {
        setFont(10);
        GFXBorderBoxPrintf(BigSingleTopRight, "");
        GFXBorderBoxPrintf(BigSingleTopLeft, "");
        AddTitleInsideBox(8, 3, BigSingleTopRight, "STW");
        AddTitleInsideBox(8, 2, BigSingleTopRight, "Kts");
        AddTitleInsideBox(8, 3, BigSingleTopLeft, "SOG");
        AddTitleInsideBox(8, 2, BigSingleTopLeft, "Kts");
        GFXBorderBoxPrintf(BigSingleDisplay, "");
      }

      SCROLLGraph(RunSetup, 0, 3, true, BigSingleDisplay, BoatData.STW, 0, 10, 9, "STW Graph ", "Kts");
      UpdateDataTwoSize(true, true, 12, 10, BigSingleTopLeft, BoatData.SOG, "%.1f");
      UpdateDataTwoSize(true, true, 12, 10, BigSingleTopRight, BoatData.STW, "%.1f");
      if (CheckButton(BigSingleDisplay)) { Display_Page = 16; }
      //        TouchCrosshair(20); quarters select big screens
      if (CheckButton(BigSingleTopLeft)) { Display_Page = 8; }
      //if (CheckButton(bottomLeftquarter)) { Display_Page = 9; }
      //if (CheckButton(bottomRightquarter)) { Display_Page = 4; }

      break;

    case 16:  //STW large
      if (RunSetup) {
        setFont(10);
        GFXBorderBoxPrintf(BigSingleTopRight, "");
        GFXBorderBoxPrintf(BigSingleTopLeft, "");
        AddTitleInsideBox(8, 3, BigSingleTopRight, "STW");
        AddTitleInsideBox(8, 2, BigSingleTopRight, "Kts");
        AddTitleInsideBox(8, 3, BigSingleDisplay, "STW");
        AddTitleInsideBox(8, 2, BigSingleDisplay, "Kts");
        AddTitleInsideBox(8, 3, BigSingleTopLeft, "SOG");
        AddTitleInsideBox(8, 2, BigSingleTopLeft, "Kts");
        GFXBorderBoxPrintf(BigSingleDisplay, "");
      }
      LocalCopy = BoatData.STW;
      UpdateDataTwoSize(3, true, true, 13, 12, BigSingleDisplay, LocalCopy, "%.1f");  // note magnify 3!! so needs the local copy
      UpdateDataTwoSize(true, true, 12, 10, BigSingleTopLeft, BoatData.SOG, "%.1f");
      UpdateDataTwoSize(true, true, 12, 10, BigSingleTopRight, BoatData.STW, "%.1f");

      if (CheckButton(BigSingleDisplay)) { Display_Page = 6; }
      //        TouchCrosshair(20); quarters select big screens
      if (CheckButton(BigSingleTopLeft)) { Display_Page = 8; }
      break;

    case 7:  // Depth  (fathmometer 30)  (circulate 7/11/17)
      if (RunSetup) {
        setFont(11);
        GFXBorderBoxPrintf(BigSingleTopRight, "");
        AddTitleInsideBox(8, 3, BigSingleTopRight, "Depth");
        AddTitleInsideBox(8, 2, BigSingleTopRight, " m");
        GFXBorderBoxPrintf(BigSingleDisplay, "");
        //AddTitleInsideBox(9,3,BigSingleDisplay, "Fathmometer 30m");
      }

      SCROLLGraph(RunSetup, 0, 3, true, BigSingleDisplay, BoatData.WaterDepth, 30, 0, 9, "Fathmometer 30m ", "m");
      UpdateDataTwoSize(true, true, 12, 10, BigSingleTopRight, BoatData.WaterDepth, "%.1f");

      if (CheckButton(BigSingleTopRight)) { Display_Page = 4; }
      //        TouchCrosshair(20); quarters select big screens
      if (CheckButton(topLeftquarter)) { Display_Page = 4; }
      if (CheckButton(BigSingleDisplay)) { Display_Page = 11; }
      break;

    case 11:  // Depth (fathmometer 1 0) different range
      if (RunSetup) {
        setFont(10);
        GFXBorderBoxPrintf(BigSingleTopRight, "");
        AddTitleInsideBox(8, 1, BigSingleTopRight, "Depth");
        AddTitleInsideBox(8, 2, BigSingleTopRight, "m");
        GFXBorderBoxPrintf(BigSingleDisplay, "");
      }

      SCROLLGraph(RunSetup, 0, 3, true, BigSingleDisplay, BoatData.WaterDepth, 10, 0, 9, "Fathmometer 10m ", "m");
      UpdateDataTwoSize(true, true, 12, 10, BigSingleTopRight, BoatData.WaterDepth, "%.1f");

      //        TouchCrosshair(20); quarters select big screens
      if (CheckButton(topLeftquarter)) { Display_Page = 4; }
      if (CheckButton(BigSingleDisplay)) { Display_Page = 17; }
      if (CheckButton(topRightquarter)) { Display_Page = 7; }
      break;

    case 17:  // Depth (big digital)
      if (RunSetup) {
        setFont(10);
        GFXBorderBoxPrintf(BigSingleTopRight, "");
        AddTitleInsideBox(8, 1, BigSingleTopRight, "Depth");
        AddTitleInsideBox(8, 2, BigSingleTopRight, "m");
        GFXBorderBoxPrintf(BigSingleDisplay, "");
        AddTitleInsideBox(10, 2, BigSingleDisplay, "m");
      }
      LocalCopy = BoatData.WaterDepth;  //WaterDepth, "%4.1f m");  // nb %4.1 will give leading printing space- giving formatting issues!
      UpdateDataTwoSize(4, true, true, 12, 10, BigSingleDisplay, LocalCopy, "%.1f");
      UpdateDataTwoSize(true, true, 12, 10, BigSingleTopRight, BoatData.WaterDepth, "%.1f");
      if (CheckButton(topLeftquarter)) { Display_Page = 4; }
      if (CheckButton(BigSingleDisplay)) { Display_Page = 7; }
      //if (CheckButton(topRightquarter)) { Display_Page = 4; }
      break;

    case 8:  //SOG  graph
      if (RunSetup || DataChanged) {
        setFont(11);
        GFXBorderBoxPrintf(BigSingleTopRight, "");
        GFXBorderBoxPrintf(BigSingleTopLeft, "");
        AddTitleInsideBox(8, 3, BigSingleTopRight, "STW");
        AddTitleInsideBox(8, 2, BigSingleTopRight, "Kts");
        AddTitleInsideBox(8, 3, BigSingleTopLeft, "SOG");
        AddTitleInsideBox(8, 2, BigSingleTopLeft, "Kts");
        GFXBorderBoxPrintf(BigSingleDisplay, "");

        DataChanged = false;
      }

      SCROLLGraph(RunSetup, 0, 3, true, BigSingleDisplay, BoatData.SOG, 0, 10, 9, "SOG Graph ", "kts");
      UpdateDataTwoSize(true, true, 12, 10, BigSingleTopRight, BoatData.STW, "%.1f");
      UpdateDataTwoSize(true, true, 12, 10, BigSingleTopLeft, BoatData.SOG, "%.1f");

      //if (CheckButton(Full0Center)) { Display_Page = 4; }
      //        TouchCrosshair(20); quarters select big screens
      if (CheckButton(BigSingleDisplay)) { Display_Page = 18; }  //18 is BIG SOG
      //if (CheckButton(bottomLeftquarter)) { Display_Page = 9; }
      if (CheckButton(BigSingleTopRight)) { Display_Page = 6; }
      if (CheckButton(BigSingleTopLeft)) { Display_Page = 4; }
      break;

    case 18:  //BIG SOG
      if (RunSetup || DataChanged) {
        setFont(11);
        GFXBorderBoxPrintf(BigSingleTopRight, "");
        GFXBorderBoxPrintf(BigSingleTopLeft, "");
        AddTitleInsideBox(8, 3, BigSingleTopRight, "STW");
        AddTitleInsideBox(8, 2, BigSingleTopRight, "Kts");
        AddTitleInsideBox(8, 3, BigSingleTopLeft, "SOG");
        AddTitleInsideBox(8, 2, BigSingleTopLeft, "Kts");
        GFXBorderBoxPrintf(BigSingleDisplay, "");
        DataChanged = false;
      }
      if (millis() > slowdown + 500) {
        slowdown = millis();
      }
      LocalCopy = BoatData.SOG;
      UpdateDataTwoSize(3, true, true, 13, 12, BigSingleDisplay, LocalCopy, "%.1f");  //note magnify 3 is a repeat display so needs the localCopy
      UpdateDataTwoSize(true, true, 12, 10, BigSingleTopRight, BoatData.STW, "%.1f");
      UpdateDataTwoSize(true, true, 12, 10, BigSingleTopLeft, BoatData.SOG, "%.1f");

      //if (CheckButton(Full0Center)) { Display_Page = 4; }
      //        TouchCrosshair(20); quarters select big screens
      if (CheckButton(BigSingleDisplay)) { Display_Page = 8; }  //18 is BIG SOG
      //if (CheckButton(bottomLeftquarter)) { Display_Page = 9; }
      if (CheckButton(BigSingleTopRight)) { Display_Page = 6; }
      if (CheckButton(BigSingleTopLeft)) { Display_Page = 4; }
      break;

    case 9:  // GPS page
      if (RunSetup) {
        setFont(8);
        GFXBorderBoxPrintf(BigSingleDisplay, "");
        GFXBorderBoxPrintf(TopHalfBigSingleTopRight, "");
        GFXBorderBoxPrintf(BottomHalfBigSingleTopRight, "");
        GFXBorderBoxPrintf(BigSingleTopLeft, "Click for graphic");
        setFont(10);
      }
      UpdateDataTwoSize(true, true, 9, 8, TopHalfBigSingleTopRight, BoatData.SOG, "SOG: %.1f kt");
      UpdateDataTwoSize(true, true, 9, 8, BottomHalfBigSingleTopRight, BoatData.COG, "COG: %.1f d");
      if (millis() > slowdown + 1000) {
        slowdown = millis();
        GFXBorderBoxPrintf(BigSingleDisplay, "");
        // do this one once a second.. I have not yet got simplified functions testing if previously displayed and greyed yet
        gfx->setTextColor(BigSingleDisplay.TextColor);
        BigSingleDisplay.PrintLine = 0;
        if (BoatData.SatsInView != NMEA0183DoubleNA) { UpdateLinef(8, BigSingleDisplay, "Satellites in view %.0f ", BoatData.SatsInView); }
        if (BoatData.GPSTime != NMEA0183DoubleNA) {
          UpdateLinef(9, BigSingleDisplay, "");
          UpdateLinef(9, BigSingleDisplay, "Date: %06i ", int(BoatData.GPSDate));
          UpdateLinef(9, BigSingleDisplay, "");
          UpdateLinef(9, BigSingleDisplay, "TIME: %02i:%02i:%02i",
                      int(BoatData.GPSTime) / 3600, (int(BoatData.GPSTime) % 3600) / 60, (int(BoatData.GPSTime) % 3600) % 60);
        }
        if (BoatData.Latitude.data != NMEA0183DoubleNA) {
          UpdateLinef(9, BigSingleDisplay, "");
          UpdateLinef(9, BigSingleDisplay, "LAT %s", LattoString(BoatData.Latitude.data));
          UpdateLinef(9, BigSingleDisplay, "LON %s", LongtoString(BoatData.Longitude.data));
          UpdateLinef(9, BigSingleDisplay, "");
        }

        if (BoatData.MagHeading.data != NMEA0183DoubleNA) { UpdateLinef(9, BigSingleDisplay, "Mag Heading: %.4f", BoatData.MagHeading); }
        UpdateLinef(9, BigSingleDisplay, "Variation: %.4f", BoatData.Variation);
      }
      if (CheckButton(BigSingleTopLeft)) { Display_Page = 10; }
      //if (CheckButton(bottomLeftquarter)) { Display_Page = 4; }  //Loop to the main settings page
      break;

    case 10:  // GPS page 2 sort of anchor watch
      static double magnification;
      if (RunSetup || DataChanged) {
        setFont(8);
        GFXBorderBoxPrintf(BigSingleDisplay, "");
        GFXBorderBoxPrintf(BigSingleTopLeft, "");
        if (BoatData.GPSTime != NMEA0183DoubleNA) {
          UpdateLinef(8, BigSingleTopLeft, "Date: %06i ", int(BoatData.GPSDate));
          UpdateLinef(8, BigSingleTopLeft, "TIME: %02i:%02i:%02i",
                      int(BoatData.GPSTime) / 3600, (int(BoatData.GPSTime) % 3600) / 60, (int(BoatData.GPSTime) % 3600) % 60);
        }
        if (BoatData.Latitude.data != NMEA0183DoubleNA) {
          UpdateLinef(8, BigSingleTopLeft, "LAT: %f", BoatData.Latitude.data);
          UpdateLinef(8, BigSingleTopLeft, "LON: %f", BoatData.Longitude.data);
        }

        GFXBorderBoxPrintf(BigSingleTopRight, "Show Quad Display");
        GFXBorderBoxPrintf(BottomRightbutton, "Zoom in");
        GFXBorderBoxPrintf(BottomLeftbutton, "Zoom out");
        magnification = 1111111;  //reset magnification 11111111 = 10 pixels / m == 18m circle.
        DataChanged = false;
      }
      if (millis() > slowdown + 1000) {
        slowdown = millis();
        // do this one once a second.. I have not yet got simplified functions testing if previously displayed and greyed yet
        ///gfx->setTextColor(BigSingleDisplay.TextColor);
        BigSingleTopLeft.PrintLine = 0;
        // UpdateLinef(3,BigSingleTopLeft, "%.0f Satellites in view", BoatData.SatsInView);
        if (BoatData.GPSTime != NMEA0183DoubleNA) {
          UpdateLinef(8, BigSingleTopLeft, "Date: %06i ", int(BoatData.GPSDate));
          UpdateLinef(8, BigSingleTopLeft, "TIME: %02i:%02i:%02i",
                      int(BoatData.GPSTime) / 3600, (int(BoatData.GPSTime) % 3600) / 60, (int(BoatData.GPSTime) % 3600) % 60);
        }
        if (BoatData.Latitude.data != NMEA0183DoubleNA) {
          UpdateLinef(8, BigSingleTopLeft, "LAT: %s", LattoString(BoatData.Latitude.data));
          UpdateLinef(8, BigSingleTopLeft, "LON: %s", LongtoString(BoatData.Longitude.data));
          DrawGPSPlot(false, BigSingleDisplay, BoatData, magnification);
        }
      }
      if (CheckButton(topLeftquarter)) { Display_Page = 9; }
      if (CheckButton(BigSingleTopRight)) { Display_Page = 4; }

      if (CheckButton(BottomRightbutton)) {
        magnification = magnification * 1.5;
        Serial.printf(" magification  %f \n", magnification);
      }
      if (CheckButton(BottomLeftbutton)) {
        magnification = magnification / 1.5;
        Serial.printf(" magification  %f \n", magnification);
      }
      if (CheckButton(BigSingleDisplay)) {  // press plot to recenter plot
        if (BoatData.Latitude.data != NMEA0183DoubleNA) {
          DrawGPSPlot(true, BigSingleDisplay, BoatData, magnification);
          Serial.printf(" reset center anchorwatch %f   %f \n", startposlat, startposlon);
          GFXBorderBoxPrintf(BigSingleDisplay, "");
          GFXBorderBoxPrintf(BottomRightbutton, "zoom in");
          GFXBorderBoxPrintf(BottomLeftbutton, "zoom out");
        }
        DataChanged = true;
      }
      break;

    case 15:  // wind instrument TRUE Ground ref - experimental
      if (RunSetup) {
        setFont(10);
        GFXBorderBoxPrintf(BigSingleDisplay, "");
        GFXBorderBoxPrintf(BigSingleTopRight, "");
        AddTitleInsideBox(8, 2, BigSingleTopRight, " deg");
        DrawCompass(BigSingleDisplay);
        AddTitleInsideBox(8, 3, BigSingleDisplay, "WIND ground ");
      }
      if (millis() > slowdown + 500) {
        slowdown = millis();
      }
      UpdateDataTwoSize(true, true, 9, 8, TopHalfBigSingleTopRight, BoatData.WindAngleApp, "app %.1f");
      WindArrow2(BigSingleDisplay, BoatData.WindSpeedK, BoatData.WindAngleGround);
      UpdateDataTwoSize(true, true, 9, 8, BottomHalfBigSingleTopRight, BoatData.WindAngleGround, "gnd %.1f");

      if (CheckButton(topLeftquarter)) { Display_Page = 4; }
      if (CheckButton(BigSingleDisplay)) { Display_Page = 5; }
      break;
    default:
      Display_Page = 0;
      break;
  }
  LastPageselected = page;
  RunSetup = false;
}

void setFont(int fontinput) {  //fonts 3..12 are FreeMonoBold in sizes incrementing by 1.5
                               //Notes: May remove some later to save program mem space?
                               // used : 0,1,2,4 for keyboard
                               //      : 0,3,4,8,10,11 in main
  MasterFont = fontinput;
  switch (fontinput) {  //select font and automatically set height/offset based on character '['
    // set the heights and offset to print [ in boxes. Heights in pixels are NOT the point heights!
    case 0:                        // SMALL 8pt
      Fontname = "FreeMono8pt7b";  //9 to 14 high?
      gfx->setFont(&FreeMono8pt7b);
      text_height = (FreeMono8pt7bGlyphs[0x3D].height) + 1;
      text_offset = -(FreeMono8pt7bGlyphs[0x3D].yOffset);
      text_char_width = 12;
      break;
    case 1:  // standard 12pt
      Fontname = "FreeMono12pt7b";
      gfx->setFont(&FreeMono12pt7b);
      text_height = (FreeMono12pt7bGlyphs[0x3D].height) + 1;
      text_offset = -(FreeMono12pt7bGlyphs[0x3D].yOffset);
      text_char_width = 12;

      break;
    case 2:  //standard 18pt
      Fontname = "FreeMono18pt7b";
      gfx->setFont(&FreeMono18pt7b);
      text_height = (FreeMono18pt7bGlyphs[0x3D].height) + 1;
      text_offset = -(FreeMono18pt7bGlyphs[0x3D].yOffset);
      text_char_width = 12;

      break;
    case 3:  //BOLD 8pt
      Fontname = "FreeMonoBOLD8pt7b";
      gfx->setFont(&FreeMonoBold8pt7b);
      text_height = (FreeMonoBold8pt7bGlyphs[0x3D].height) + 1;
      text_offset = -(FreeMonoBold8pt7bGlyphs[0x3D].yOffset);
      text_char_width = 12;

      break;
    case 4:  //BOLD 12pt
      Fontname = "FreeMonoBOLD12pt7b";
      gfx->setFont(&FreeMonoBold12pt7b);
      text_height = (FreeMonoBold12pt7bGlyphs[0x3D].height) + 1;
      text_offset = -(FreeMonoBold12pt7bGlyphs[0x3D].yOffset);
      text_char_width = 12;

      break;
    case 5:  //BOLD 18 pt
      Fontname = "FreeMonoBold18pt7b";
      gfx->setFont(&FreeMonoBold18pt7b);
      text_height = (FreeMonoBold18pt7bGlyphs[0x3D].height) + 1;
      text_offset = -(FreeMonoBold18pt7bGlyphs[0x3D].yOffset);
      text_char_width = 12;
      break;
    case 6:  //BOLD 27 pt
      Fontname = "FreeMonoBold27pt7b";
      gfx->setFont(&FreeMonoBold27pt7b);
      text_height = (FreeMonoBold27pt7bGlyphs[0x3D].height) + 1;
      text_offset = -(FreeMonoBold27pt7bGlyphs[0x3D].yOffset);
      text_char_width = 12;
      break;
    case 7:  //SANS BOLD 6 pt
      Fontname = "FreeSansBold6pt7b";
      gfx->setFont(&FreeSansBold6pt7b);
      text_height = (FreeSansBold6pt7bGlyphs[0x38].height) + 1;
      text_offset = -(FreeSansBold6pt7bGlyphs[0x38].yOffset);
      text_char_width = 12;
      break;
    case 8:  //SANS BOLD 8 pt
      Fontname = "FreeSansBold8pt7b";
      gfx->setFont(&FreeSansBold8pt7b);
      text_height = (FreeSansBold8pt7bGlyphs[0x38].height) + 1;
      text_offset = -(FreeSansBold8pt7bGlyphs[0x38].yOffset);  // yAdvance is the last variable.. and the one that affects the extra lf on wrap.
      text_char_width = 12;
      break;
    case 9:  //SANS BOLD 12 pt
      Fontname = "FreeSansBold12pt7b";
      gfx->setFont(&FreeSansBold12pt7b);
      text_height = (FreeSansBold12pt7bGlyphs[0x38].height) + 1;
      text_offset = -(FreeSansBold12pt7bGlyphs[0x38].yOffset);
      text_char_width = 12;
      break;
    case 10:  //SANS BOLD 18 pt
      Fontname = "FreeSansBold18pt7b";
      gfx->setFont(&FreeSansBold18pt7b);
      text_height = (FreeSansBold18pt7bGlyphs[0x38].height) + 1;
      text_offset = -(FreeSansBold18pt7bGlyphs[0x38].yOffset);
      text_char_width = 12;
      break;
    case 11:  //sans BOLD 27 pt
      Fontname = "FreeSansBold27pt7b";
      gfx->setFont(&FreeSansBold27pt7b);
      text_height = (FreeSansBold27pt7bGlyphs[0x38].height) + 1;
      text_offset = -(FreeSansBold27pt7bGlyphs[0x38].yOffset);
      text_char_width = 12;
      break;
    case 12:  //sans BOLD 40 pt
      Fontname = "FreeSansBold40pt7b";
      gfx->setFont(&FreeSansBold40pt7b);
      text_height = (FreeSansBold40pt7bGlyphs[0x38].height) + 1;
      text_offset = -(FreeSansBold40pt7bGlyphs[0x38].yOffset);
      text_char_width = 12;
      break;

    case 13:  //sans BOLD 60 pt
      Fontname = "FreeSansBold60pt7b";
      gfx->setFont(&FreeSansBold60pt7b);
      text_height = (FreeSansBold60pt7bGlyphs[0x38].height) + 1;
      text_offset = -(FreeSansBold60pt7bGlyphs[0x38].yOffset);
      text_char_width = 12;
      break;

      //   case 21:  //Mono oblique BOLD 27 pt
      //   Fontname = "FreeMonoBoldOblique27pt7b";
      //   gfx->setFont(&FreeMonoBoldOblique27pt7b);
      //   text_height = (FreeMonoBoldOblique27pt7bGlyphs[0x38].height) + 1;
      //   text_offset = -(FreeMonoBoldOblique27pt7bGlyphs[0x38].yOffset);
      //   text_char_width = 12;
      //   break;
      // case 22:  //Mono oblique BOLD 40 pt
      //   Fontname = "FreeMonoBoldOblique40pt7b";
      //   gfx->setFont(&FreeMonoBoldOblique40pt7b);
      //   text_height = (FreeMonoBoldOblique40pt7bGlyphs[0x38].height) + 1;
      //   text_offset = -(FreeMonoBoldOblique40pt7bGlyphs[0x38].yOffset);
      //   text_char_width = 12;
      //   break;
      //       case 23:  //Mono oblique BOLD 60 pt
      //   Fontname = "FreeMonoBoldOblique60pt7b";
      //   gfx->setFont(&FreeMonoBoldOblique60pt7b);
      //   text_height = (FreeMonoBoldOblique60pt7bGlyphs[0x38].height) + 1;
      //   text_offset = -(FreeMonoBoldOblique60pt7bGlyphs[0x38].yOffset);
      //   text_char_width = 12;
      //   break;


    default:
      Fontname = "FreeMono8pt7b";
      gfx->setFont(&FreeMono8pt7b);
      text_height = (FreeMono8pt7bGlyphs[0x3D].height) + 1;
      text_offset = -(FreeMono8pt7bGlyphs[0x3D].yOffset);
      text_char_width = 12;
      MasterFont = 0;

      break;
  }
}

void setup() {
  //CONFIG_ESP_BROWNOUT_DET_LVL_SEL_5 ??
  Serial.begin(115200);
  Serial.println("Starting NMEA Display ");
  Serial.println(soft_version);

  ts.begin();
  Serial.println("ts has begun");
  ts.setRotation(ROTATION_INVERTED);
  // guitron sets GFX_BL 38
  Serial.println("GFX_BL set");
  #ifdef GFX_BL
  pinMode(GFX_BL, OUTPUT);
  digitalWrite(GFX_BL, HIGH);
  #endif
  // Init Display
  gfx->begin();
  //if GFX> 1.3.1 try and do this as the invert colours write 21h or 20h to 0Dh has been lost from the structure!
  gfx->invertDisplay(false);
  gfx->fillScreen(BLUE);
  gfx->setTextBound(0, 0, 480, 480);
  gfx->setTextColor(WHITE);
  setFont(4);
  gfx->setCursor(40, 20);
  gfx->println(F("***Display Started***"));
  SD_Setup();
  Audio_setup();  // Puts LOGO on screen
                  // try earlier? one source says audio needs to be started before gfx display //SD_Setup();Audio_setup();

  // gfx->setCursor(140, 140);
  // gfx->println(soft_version);


  EEPROM_READ();  // setup and read Saved_Settings (saved variables)
  Current_Settings = Saved_Settings;
  // Should automatically load default config if run for the first time
  // JSON READ will Overwrite EEPROM settings!
  //      But will update JSON IF the Data is changed (I have a JSON save in EEPROM WRITE)
  //Serial.println(F("Loading JSON configuration..."));
  if (LoadConfiguration(Setupfilename, Display_Config, Current_Settings)) {
    Serial.println(" USING JSON for wifi and display settings");
  } else {
    Display_Config = Default_JSON;
    Serial.println(" USING EEPROM data, display set to defaults");
  }

  if (LoadVictronConfiguration(VictronDevicesSetupfilename, victronDevices)) {
    Serial.println(" USING JSON for Victron data settings");
  } else {
    Serial.println("\n\n***FAILED TO GET Victron JSON FILE****\n**** SAVING DEFAULT and Making File on SD****\n\n");
    Num_Victron_Devices = 6;
    SaveVictronConfiguration(VictronDevicesSetupfilename, victronDevices);  // should write a default file if it was missing?
  }
  if (LoadDisplayConfiguration(ColorsFilename, ColorSettings)) {
    Serial.println(" USING JSON for Colours data settings");
  } else {
    Serial.println("\n\n***FAILED TO GET Colours JSON FILE****\n**** SAVING DEFAULT and Making File on SD****\n\n");
    SaveDisplayConfiguration(ColorsFilename, ColorSettings);  // should write a default file if it was missing?
  }

  // set up anything BoatData from the configs
  //Serial.print("now.. magvar:");Serial.println(BoatData.Variation);


  // flash User selected logo and setup audio if SD present
  if (hasSD) {
    // // flash logo
    // Serial.printf("display <%s> \n",Display_Config.StartLogo);
    jpegDraw(JPEG_FILENAME_LOGO, jpegDrawCallback, true /* useBigEndian */,
             // jpegDraw(StartLogo, jpegDrawCallback, true /* useBigEndian */,
             0 /* x */, 0 /* y */, gfx->width() /* widthLimit */, gfx->height() /* heightLimit */);
    setFont(11);
    gfx->setTextBound(0, 0, 480, 480);
    gfx->setCursor(30, 80);
    gfx->setTextColor(BLACK);
    gfx->println(soft_version);
    gfx->setCursor(35, 75);
    gfx->setTextColor(WHITE);
    gfx->println(soft_version);
    delay(500);
    //
  }
  gfx->setCursor(140, 240);
  // print config files
  PrintJsonFile(" Display and wifi config file...", Setupfilename);
  PrintJsonFile(" Victron JSON config file..", VictronDevicesSetupfilename);
  PrintJsonFile(" Display colour  config file..", ColorsFilename);
  ConnectWiFiusingCurrentSettings();
  SetupWebstuff();

  keyboard(-1);  //reset keyboard display update settings
  Udp.begin(atoi(Current_Settings.UDP_PORT));
  //delay(1000);       // time to admire your user page!
  gfx->setTextBound(0, 0, 480, 480);
  gfx->setTextColor(WHITE);
  Display_Page = Display_Config.Start_Page;  // select first page from the JSON. to show or use non defined page to start with default
  Serial.printf(" Starting display page<%i> \n", Display_Config.Start_Page);
  Start_ESP_EXT();  //  Sets esp_now links to the current WiFi.channel etc.
  BLEsetup();       // setup Victron BLE interface (does not do much!!)
}
//unsigned long Interval;  // may also be used in sub functions during debug chasing delays.. Serial.printf(" s<%i>",millis()-Interval);Interval=millis();

void timeupdate() {
  static unsigned long tick;
  while ((millis() >= tick)) {
    tick = tick + 1000;  // not millis or you can get 'slip'
    BoatData.LOCTime = BoatData.LOCTime + 1;
  }
}

double ValidData(_sInstData variable) {  // To avoid showing NMEA0183DoubleNA value in displays etc replace with zero.
  double res = 0;
  if (variable.greyed) { return 0; }
  if (variable.data != NMEA0183DoubleNA) { res = variable.data; }
  return res;
}

void loop() {
  static unsigned long LogInterval;
  static unsigned long DebugInterval;
  static unsigned long SSIDSearchInterval;
  timeupdate();
  //Serial.printf(" s<%i>",millis()-Interval);Interval=millis();
  yield();
  server.handleClient();  // for OTA;

  delay(1);
  ts.read();
  CheckAndUseInputs();
  Display(Display_Page);
  /*BLEloop*/
  if ((Current_Settings.BLE_enable) && ((Display_Page == -86) || (Display_Page == -87))) {
    if (audio.isRunning()) {
      delay(10);
      WifiGFXinterrupt(8, WifiStatus, "BLE will start\nwhen Audio finished");
    } else {
      BLEloop();
    }
  }  // Prioritize the sounds if on..
     //ONLY on  victron Display_Page -86 and -87!! or it interrupts eveything!
  EXTHeartbeat();
  audio.loop();

  if (!AttemptingConnect && !IsConnected && (millis() >= SSIDSearchInterval)) {  // repeat at intervals to check..
    SSIDSearchInterval = millis() + scansearchinterval;                          //
    if (StationsConnectedtomyAP == 0) {                                          // avoid scanning if we have someone connected to AP as it will/may disconnect!
      ScanAndConnect(true);
    }  // ScanAndConnect will set AttemptingConnect And do a Wifi.begin if the required SSID has appeared
  }

  // NMEALOG is done in CheckAndUseInputs
  // the instrument log saves everything every LogInterval (set in config)  secs, even if data is not available! (NMEA0183DoubleNA)
  // uses LOCAL Time as this advances if GPS (UTC) is lost (but resets when GPS received again.)
  if ((Current_Settings.Log_ON) && (millis() >= LogInterval)) {
    LogInterval = millis() + (Current_Settings.log_interval_setting * 1000);
    INSTLOG("%02i:%02i:%02i ,%4.2f STW ,%4.2f head-mag, %4.2f SOG ,%4.2f COG ,%4.2f DPT, %4.2f WS,%3.1f WA \r\n",
            int(BoatData.LOCTime) / 3600, (int(BoatData.LOCTime) % 3600) / 60, (int(BoatData.LOCTime) % 3600) % 60,
            ValidData(BoatData.STW), ValidData(BoatData.MagHeading), ValidData(BoatData.SOG), ValidData(BoatData.COG),
            ValidData(BoatData.WaterDepth), ValidData(BoatData.WindSpeedK), ValidData(BoatData.WindAngleApp));
  }
  // switch off WIFIGFXBox after timed interval
  if (WIFIGFXBoxdisplaystarted && (millis() >= WIFIGFXBoxstartedTime + 10000) && (!AttemptingConnect)) {
    WIFIGFXBoxdisplaystarted = false;
    Display(true, Display_Page);
    delay(50);  // change page back, having set zero above which alows the graphics to reset up the boxes etc.
  }

  if ((ColorSettings.Debug) && (millis() >= DebugInterval)) {
    DebugInterval = millis() + 1000;
    size_t current_free_heap = esp_get_free_heap_size();
    Serial.printf("Current Free Heap Size: %zu bytes, ", current_free_heap);
    // Get the minimum free heap size seen so far
    size_t min_free_heap = esp_get_minimum_free_heap_size();
    Serial.printf("Minimum Free Heap Size: %zu bytes\n", min_free_heap);
    // Add a small delay to ensure the values are stable
    delay(1);
  }
}


// useful when debugging to see where touch is
void TouchCrosshair(int size) {
  for (int i = 0; i < (ts.touches); i++) {
    TouchCrosshair(i, size, WHITE);
  }
}
void TouchCrosshair(int point, int size, uint16_t colour) {
  gfx->setCursor(ts.points[point].x, ts.points[point].y);
  // gfx->printf("%i", point);
  gfx->drawFastVLine(ts.points[point].x, ts.points[point].y - size, 2 * size, colour);
  gfx->drawFastHLine(ts.points[point].x - size, ts.points[point].y, 2 * size, colour);
}

bool CheckButton(_sButton& button) {  // trigger on release. needs index (s) to remember which button!
  //trigger on release! does not sense !isTouched ..  use Keypressed in each button struct to keep track!
  if (ts.isTouched && !button.Keypressed && (millis() - button.LastDetect >= 250)) {
    if (XYinBox(ts.points[0].x, ts.points[0].y, button.h, button.v, button.width, button.height)) {
      //Serial.printf(" Checkbutton size%i state %i %i \n",ts.points[0].size,ts.isTouched,XYinBox(ts.points[0].x, ts.points[0].y,button.h,button.v,button.width,button.height));
      button.Keypressed = true;
      button.LastDetect = millis();
    }
    return false;
  }
  if (button.Keypressed && (millis() - button.LastDetect >= 250)) {
    //Serial.printf(" Checkbutton released from  %i %i\n",button.h,button.v);
    button.Keypressed = false;
    return true;
  }
  return false;
}
void CheckAndUseInputs() {  //multiinput capable, will check sources in sequence
  static unsigned long MAXScanInterval;
  MAXScanInterval = millis() + 500;
  // Serial.printf(" C&U<%i>",millis()-Interval);Interval=millis();
  if ((Current_Settings.ESP_NOW_ON)) {  // ESP_now can work even if not actually 'connected', so for now, do not risk the while loop!
                                        // old.. only did one line of nmea_EXT..
                                        // if (nmea_EXT[0] != 0) { UseNMEA(nmea_EXT, 3); }
    while (Test_ESP_NOW() && (millis() <= MAXScanInterval)) {
      UseNMEA(nmea_EXT, 3);
      // runs multiple times to clear the buffer.. use delay to allow other things to work.. print to show if this is the cause of start delays while debugging!
      audio.loop();
      vTaskDelay(1);
    }
  }
  // Serial.printf(" ca<%i>",millis()-Interval);Interval=millis();
  if (Current_Settings.Serial_on) {
    if (Test_Serial_1()) { UseNMEA(nmea_1, 1); }
  }
  // Serial.printf(" cb<%i>",millis()-Interval);Interval=millis();
  if (Current_Settings.UDP_ON) {
    if (Test_U()) { UseNMEA(nmea_U, 2); }
  }
  //Serial.printf(" cd<%i>",millis()-Interval);Interval=millis();
  if (Current_Settings.BLE_enable) {
    if (VictronBuffer[0] != 0) { UseNMEA(VictronBuffer, 4); }
  }
  // Serial.printf(" ce<%i>\n",millis()-Interval);Interval=millis();
}

void UseNMEA(char* buf, int type) {
  if (buf[0] != 0) {
    // print serial version if on the wifi page terminal window page.
    // data log raw NMEA and when and where it came from.
    // type 4 is Victron data
    /*TIME: %02i:%02i:%02i",
                      int(BoatData.GPSTime) / 3600, (int(BoatData.GPSTime) % 3600) / 60,*/
    if (Current_Settings.NMEA_log_ON) {
      if (BoatData.GPSTime != NMEA0183DoubleNA) {
        if (type == 1) { NMEALOG(" %02i:%02i:%02i UTC: SER:%s", int(BoatData.GPSTime) / 3600, (int(BoatData.GPSTime) % 3600) / 60, (int(BoatData.GPSTime) % 3600) % 60, buf); }
        if (type == 2) { NMEALOG(" %02i:%02i:%02i UTC: UDP:%s", int(BoatData.GPSTime) / 3600, (int(BoatData.GPSTime) % 3600) / 60, (int(BoatData.GPSTime) % 3600) % 60, buf); }
        if (type == 3) { NMEALOG(" %02i:%02i:%02i UTC: ESP:%s", int(BoatData.GPSTime) / 3600, (int(BoatData.GPSTime) % 3600) / 60, (int(BoatData.GPSTime) % 3600) % 60, buf); }
        if (type == 4) { NMEALOG("\n%.3f BLE: Victron:%s", float(millis()) / 1000, buf); }

      } else {

        if (type == 1) { NMEALOG("%.3f SER:%s", float(millis()) / 1000, buf); }
        if (type == 2) { NMEALOG("%.3f UDP:%s", float(millis()) / 1000, buf); }
        if (type == 3) { NMEALOG("%.3f ESP:%s", float(millis()) / 1000, buf); }
        if (type == 4) { NMEALOG("\n %.3f VIC:%s", float(millis()) / 1000, buf); }
      }
    }
    // 8 is snasBold8pt small font and seems to wrap to give a space before the second line
    // 7 is smallest
    // 0 is 8pt mono thin,
    //3 is 8pt mono bold
    if ((Display_Page == -86)) {  //Terminal.debugpause built into in UpdateLinef as part of button characteristics
      if (type == 4) {
        UpdateLinef(BLACK, 8, Terminal, "V_Debugmsg%s", buf);  //8 readable ! 7 small enough to avoid line wrap issue?
      }
    }

    if ((Display_Page == -21)) {  //Terminal.debugpause built into in UpdateLinef as part of button characteristics
      if (type == 4) {
        UpdateLinef(BLACK, 8, Terminal, "Victron:%s", buf);  // 7 small enough to avoid line wrap issue?
      }

      if (type == 2) {
        UpdateLinef(BLUE, 8, Terminal, "UDP:%s", buf);  // 7 small enough to avoid line wrap issue?
      }
      if (type == 3) {
        UpdateLinef(RED, 8, Terminal, "ESP:%s", buf);
      }
      if (type == 1) { UpdateLinef(GREEN, 8, Terminal, "Ser:%s", buf); }
    }
    // now decode it for the displays to use
    if (type != 4) {
      pTOKEN = buf;                                               // pToken is used in processPacket to separate out the Data Fields
      if (processPacket(buf, BoatData)) { dataUpdated = true; };  // NOTE processPacket will search for CR! so do not remove it and then do page updates if true ?
    }
    /// WILL NEED new process packet equivalent to deal with VICTRON data
    buf[0] = 0;  //clear buf  when finished!
    return;
  }
}

//*********** EEPROM functions *********
void EEPROM_WRITE(_sDisplay_Config B, _sWiFi_settings_Config A) {
  // save my current settings
  // ALWAYS Write the Default display page!  may change this later and save separately?!!
  Serial.printf("SAVING EEPROM\n key:%i \n", A.EpromKEY);
  EEPROM.put(1, A.EpromKEY);  // separate and duplicated so it can be checked by itsself first in case structures change
  EEPROM.put(10, A);
  EEPROM.commit();
  delay(50);
  //NEW also save as a JSON on the SD card SD card will overwrite current settings on setup..
  SaveConfiguration(Setupfilename, B, A);
  // SaveVictronConfiguration(VictronDevicesSetupfilename,victronDevices); // should write a default file if it was missing?
}
void EEPROM_READ() {
  int key;
  EEPROM.begin(512);
  Serial.print("READING EEPROM ");
  gfx->println(" EEPROM READING ");
  EEPROM.get(1, key);
  // Serial.printf(" read %i  default %i \n", key, Default_Settings.EpromKEY);
  if (key == Default_Settings.EpromKEY) {
    EEPROM.get(10, Saved_Settings);
    Serial.println("EEPROM Key OK");
    gfx->println("EEPROM Key OK");
  } else {
    Saved_Settings = Default_Settings;
    gfx->println("Using DEFAULTS");
    Serial.println("Using DEFAULTS");
    EEPROM_WRITE(Default_JSON, Default_Settings);
  }
}


boolean CompStruct(_sWiFi_settings_Config A, _sWiFi_settings_Config B) {  // Does NOT compare the display page number or key!
  bool same = true;
  // have to check each variable individually
  //if (A.EpromKEY == B.EpromKEY) { same = true; }

  if (A.UDP_ON != B.UDP_ON) { same = false; }
  if (A.ESP_NOW_ON != B.ESP_NOW_ON) { same = false; }
  if (A.Serial_on != B.Serial_on) { same = false; }
  if (A.Log_ON != B.Log_ON) { same = false; }
  if (A.NMEA_log_ON != B.NMEA_log_ON) { same = false; }

  //Serial.print(" DEBUG ");Serial.print(A.ssid); Serial.print(" and ");Serial.println(B.ssid);
  // these are char strings, so need strcmp to compare ...if strcmp==0 they are equal
  if (strcmp(A.UDP_PORT, B.UDP_PORT) != 0) { same = false; }
  if (strcmp(A.ssid, B.ssid) != 0) { same = false; }
  if (strcmp(A.password, B.password) != 0) { same = false; }

  //Serial.print("Result same = ");Serial.println(same);
  return same;
}



//********* Send Advice function - useful for messages can be switched on/off here for debugging
void sendAdvice(String message) {  // just a general purpose advice send that makes sure it sends at 115200

  Serial.print(message);
}
void sendAdvicef(const char* fmt, ...) {  //complete object type suitable for holding the information needed by the macros va_start, va_copy, va_arg, and va_end.
  static char msg[300] = { '\0' };        // used in message buildup
  va_list args;
  va_start(args, fmt);
  vsnprintf(msg, 128, fmt, args);
  va_end(args);
  // add checksum?
  int len = strlen(msg);
  sendAdvice(msg);
  delay(10);  // let it send!
}




//****           SD and image functions  include #include "JpegFunc.h"


static int jpegDrawCallback(JPEGDRAW* pDraw) {
  // Serial.printf("Draw pos = %d,%d. size = %d x %d\n", pDraw->x, pDraw->y, pDraw->iWidth, pDraw->iHeight);
  gfx->draw16bitBeRGBBitmap(pDraw->x, pDraw->y, pDraw->pPixels, pDraw->iWidth, pDraw->iHeight);
  return 1;
}

void listDir(fs::FS& fs, const char* dirname, uint8_t levels) {
  Serial.printf("Listing directory: %s\n", dirname);
  gfx->printf("Listing directory: %s\n", dirname);
  File root = fs.open(dirname);
  if (!root) {
    Serial.println("Failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println("Not a directory");
    return;
  }

  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      Serial.print("  DIR : ");
      Serial.println(file.name());
      gfx->print("  DIR : ");
      gfx->println(file.name());
      if (levels) {
        listDir(fs, file.path(), levels - 1);
      }
    } else {
      Serial.print("  FILE: ");
      Serial.print(file.name());
      gfx->print("  FILE : ");
      gfx->println(file.name());
      Serial.print("  SIZE: ");
      Serial.println(file.size());
    }
    file = root.openNextFile();
  }
}




void SD_Setup() {
  hasSD = false;
  Serial.println("SD Card START");
  SPI.begin(SD_SCK, SD_MISO, SD_MOSI);
  delay(100);
  if (!SD.begin(SD_CS)) {
    Serial.println("Card Mount Failed");
    gfx->println("NO SD Card");
    return;
  } else {
    hasSD = true;  // picture is run in setup, after load config
  }

  uint8_t cardType = SD.cardType();

  if (cardType == CARD_NONE) {
    Serial.println("No SD card attached");
    return;
  }
  Serial.print("SD Card Type: ");
  gfx->println(" ");  // or it starts outside text bound??
  gfx->print("SD Card Type: ");
  if (cardType == CARD_MMC) {
    Serial.println("MMC");
    gfx->println("MMC");
  } else if (cardType == CARD_SD) {
    Serial.println("SDSC");
    gfx->println("SCSC");
  } else if (cardType == CARD_SDHC) {
    Serial.println("SDHC");
    gfx->println("SDHC");
  } else {
    Serial.println("UNKNOWN");
    gfx->println("Unknown");
  }

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);
  gfx->printf("SD Card Size: %lluMB\n", cardSize);
  // Serial.println("*** SD card contents  (to three levels) ***");
  //listDir(SD, "/", 3);
}
//  ************  WIFI support functions *****************

void WifiGFXinterrupt(int font, _sButton& button, const char* fmt, ...) {  //quick interrupt of gfx to show WIFI events..
  if (Display_Page <= -1) { return; }                                      // do not interrupt the settings pages!                                                                       // version of add centered text, multi line from /void MultiLineInButton(int font, _sButton &button,const char *fmt, ...)
  static char msg[300] = { '\0' };
  va_list args;
  va_start(args, fmt);
  vsnprintf(msg, 128, fmt, args);
  va_end(args);
  int len = strlen(msg);
  static char* token;
  const char delimiter[2] = "\n";  //  NB when i used  "static const char delimiter = '\n';"  I got big problems ..
  char* pch;
  GFXBorderBoxPrintf(button, "");  // clear the button
  pch = strtok(msg, delimiter);    // split (tokenise)  msg at the delimiter
  // print each separated line centered... starting from line 1
  button.PrintLine = 1;
  while (pch != NULL) {
    CommonSub_UpdateLine(button.TextColor, font, button, pch);
    pch = strtok(NULL, delimiter);
  }
  WIFIGFXBoxdisplaystarted = true;
  WIFIGFXBoxstartedTime = millis();
}

void wifiEvent(WiFiEvent_t event, WiFiEventInfo_t info) {

  switch (event) {
    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
      Serial.println("WiFi connected");
      //  gfx->println(" Connected ! ");
      Serial.print("** Connected to : ");
      IsConnected = true;
      AttemptingConnect = false;
      //  gfx->println(" Using :");
      //  gfx->println(WiFi.SSID());
      Serial.print(WiFi.SSID());
      Serial.println(">");
      WifiGFXinterrupt(9, WifiStatus, "CONNECTED TO\n<%s>", WiFi.SSID());
      break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      // take care with printf. It can quickly crash if it gets stuff it cannot deal with.
      //  Serial.printf(" Disconnected.reason %s  isConnected%s   attemptingConnect%s  \n",disconnectreason(info.wifi_sta_disconnected.reason).c_str(),IsConnected On_Off,AttemptingConnect On_Off);
      if (!IsConnected) {
        if (AttemptingConnect) { return; }
        Serial.println("WiFi disconnected");
        Serial.print("WiFi lost reason: ");
        Serial.println(disconnectreason(info.wifi_sta_disconnected.reason));
        if (ScanAndConnect(true)) {  // is the required SSID to be found?
          WifiGFXinterrupt(8, WifiStatus, "Attempting Reconnect to\n<%s>", Current_Settings.ssid);
          Serial.println("Attempting Reconnect");
        }
      } else {
        Serial.println("WiFi Disconnected");
        Serial.print("WiFi Lost Reason: ");
        Serial.println(disconnectreason(info.wifi_sta_disconnected.reason));
        WiFi.disconnect(false);     // changed to false.. Revise?? so that it does this only if no one is connected to the AP ??
        AttemptingConnect = false;  // so that ScanandConnect can do a full scan next time..
        WifiGFXinterrupt(8, WifiStatus, "Disconnected \n REASON:%s\n Retrying:<%s>", disconnectreason(info.wifi_sta_disconnected.reason).c_str(), Current_Settings.ssid);
        IsConnected = false;
      }
      break;

    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      Serial.print("The ESP32 has received IP address :");
      Serial.println(WiFi.localIP());
      if (hasSD) { audio.connecttoFS(SD, "/StartSound.mp3"); }
      WifiGFXinterrupt(9, WifiStatus, "CONNECTED TO\n<%s>\nIP:%i.%i.%i.%i\n", WiFi.SSID(),
                       WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3]);
      break;

    case ARDUINO_EVENT_WIFI_AP_START:
      WifiGFXinterrupt(8, WifiStatus, "Soft-AP started\n%s ", WiFi.softAPSSID());
      Serial.println("   10 ESP32 soft-AP start");
      break;


    case ARDUINO_EVENT_WIFI_AP_STACONNECTED:  //12 a station connected to ESP32 soft-AP
      StationsConnectedtomyAP = StationsConnectedtomyAP + 1;
      WifiGFXinterrupt(8, WifiStatus, "Station Connected\nTo AP\n Total now %i", StationsConnectedtomyAP);
      break;
    case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:  //13 a station disconnected from ESP32 soft-AP
      StationsConnectedtomyAP = StationsConnectedtomyAP - 1;
      if (StationsConnectedtomyAP == 0) {}
      WifiGFXinterrupt(8, WifiStatus, "Station Disconnected\nfrom AP\n Total now %i", StationsConnectedtomyAP);

      break;
    case ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED:  //14 ESP32 soft-AP assign an IP to a connected station
      WifiGFXinterrupt(8, WifiStatus, "Station Connected\nTo AP\nNow has Assigned IP");
      Serial.print("   AP IP address: ");
      Serial.println(WiFi.softAPIP());
      break;
  }
}

bool ScanAndConnect(bool display) {
  static unsigned long ScanInterval;
  static bool found;
  unsigned long ConnectTimeout;
  // do the WIfI/scan(i) and it is independently stored somewhere!!
  // but do not call too often - give it time to run!!
  if (millis() >= ScanInterval) {
    ScanInterval = millis() + 20000;
    found = false;
    NetworksFound = WiFi.scanNetworks(false, false, true, 250, 0, nullptr, nullptr);
    delay(100);
    Serial.printf(" Scan found <%i> networks:\n", NetworksFound);
  } else {
    Serial.printf(" Using saved Scan of <%i> networks:\n", NetworksFound);
  }

  WiFi.disconnect(false);  // Do NOT turn off wifi if the network disconnects
  int channel = 0;
  long rssiValue;
  for (int i = 0; i < NetworksFound; ++i) {
    if (WiFi.SSID(i).length() <= 25) {
      Serial.printf(" <%s> ", WiFi.SSID(i));
    } else {
      Serial.printf(" <name too long> ");
    }
    if (WiFi.SSID(i) == Current_Settings.ssid) {
      found = true;
      channel = i;
      rssiValue = WiFi.RSSI(i);
    }
    Serial.printf("CH:%i signal:%i \n", WiFi.channel(i), WiFi.RSSI(i));
  }
  if (found) {
    if (display) { WifiGFXinterrupt(8, WifiStatus, "WIFI scan found <%i> networks\n Connecting to <%s> signal:%i\nplease wait", NetworksFound, Current_Settings.ssid, rssiValue); }
    Serial.printf(" Scan found <%s> \n", Current_Settings.ssid);  //gfx->printf("Found <%s> network!\n", Current_Settings.ssid);
    ConnectTimeout = millis() + 3000;
    WiFi.begin(Current_Settings.ssid, Current_Settings.password, channel);  // faster if we pre-set it the channel??
    IsConnected = false;
    AttemptingConnect = true;
    // keep printing .. inside the box?
    gfx->setTextBound(WifiStatus.h + WifiStatus.bordersize, WifiStatus.v + WifiStatus.bordersize, WifiStatus.width - (2 * WifiStatus.bordersize), WifiStatus.height - (2 * WifiStatus.bordersize));
    gfx->setTextWrap(true);
    while ((WiFi.status() != WL_CONNECTED) && (millis() <= ConnectTimeout)) {
      gfx->print('.');
      Serial.print('.');
      delay(1000);
    }
    if (WiFi.status() != WL_CONNECTED) { gfx->print("Timeout - will try later"); }
    gfx->setTextBound(0, 0, 480, 480);
  } else {
    AttemptingConnect = false;
    if (display) { WifiGFXinterrupt(8, WifiStatus, "%is WIFI scan found\n <%i> networks\n but not %s\n Will look again in %i seconds", millis() / 1000, NetworksFound, Current_Settings.ssid, scansearchinterval / 1000); }
  }
  return found;
}

void ConnectWiFiusingCurrentSettings() {
  bool result;
  uint32_t StartTime = millis();
  // superceded by WIFI box display "setting up AP" gfx->println("Setting up WiFi");
  WiFi.disconnect(false, true);  // clean the persistent memory in case someone else set it !! eg ESPHOME!!
  delay(100);
  WiFi.persistent(false);
  WiFi.mode(WIFI_AP_STA);
  // WiFi.onEvent(WiFiEventPrint); // serial print for debugging
  WiFi.onEvent(wifiEvent);  // Register the event handler
                            // start the display's AP - potentially with NULL pasword
  if ((String(Display_Config.APpassword) == "NULL") || (String(Display_Config.APpassword) == "null") || (String(Display_Config.APpassword) == "")) {
    result = WiFi.softAP(Display_Config.PanelName);
    delay(5);
  } else {
    result = WiFi.softAP(Display_Config.PanelName, Display_Config.APpassword);
  }
  delay(5);
  if (result == true) {
    Serial.println("Soft-AP creation success!");
    Serial.print("   ssidAP: ");

    Serial.println(WiFi.softAPSSID());
    Serial.print("   passAP: ");
    Serial.println(Display_Config.APpassword);
    Serial.print("   AP IP address: ");
    Serial.println(WiFi.softAPIP());
  } else {
    Serial.println("Soft-AP creation failed! set this up..");
    Serial.print("   ssidAP: ");
    Serial.println(WiFi.softAPSSID());
    Serial.print("   AP IP address: ");
    Serial.println(WiFi.softAPIP());
  }
  WiFi.mode(WIFI_AP_STA);
  // all Serial prints etc are now inside ScanAndConnect 'TRUE' will display them.
  if (ScanAndConnect(true)) {  //Serial.println("found SSID and attempted connect");
    if (WiFi.status() != WL_CONNECTED) { WifiGFXinterrupt(8, WifiStatus, "Time %is \nWIFI scan found\n <%i> networks\n but did not connect to\n <%s> \n Will look again in 30 seconds",
                                                          millis() / 1000, NetworksFound, Current_Settings.ssid); }
  }
}

bool Test_Serial_1() {  // UART0 port P1
  static bool LineReading_1 = false;
  static int Skip_1 = 1;
  static int i_1;
  static bool line_1;  //has found a full line!
  unsigned char b;
  if (!line_1) {                  // ONLY get characters if we are NOT still processing the last line message!
    while (Serial.available()) {  // get the character
      b = Serial.read();
      if (LineReading_1 == false) {
        nmea_1[0] = b;
        i_1 = 1;
        LineReading_1 = true;
      }  // Place first character of line in buffer location [0]
      else {
        nmea_1[i_1] = b;
        i_1 = i_1 + 1;
        if (b == 0x0A) {       //0A is LF
          nmea_1[i_1] = 0x00;  // put end in buffer.
          LineReading_1 = false;
          line_1 = true;
          return true;
        }
        if (i_1 > 150) {
          LineReading_1 = false;
          i_1 = 0;
          line_1 = false;
          return false;
        }
      }
    }
  }
  line_1 = false;
  return false;
}
bool Test_U() {  // check if udp packet (UDP is sent in lines..) has arrived
  static int Skip_U = 1;
  // if (!line_U) {  // only process if we have dealt with the last line.
  nmea_U[0] = 0x00;
  int packetSize = Udp.parsePacket();
  if (packetSize) {  // Deal with UDP packet
    if (packetSize >= (BufferLength)) {
#if ESP_ARDUINO_VERSION_MAJOR == 3
      Udp.clear();
#else
      Udp.flush();
#endif
      return false;
    }  // Simply discard if too long
    int len = Udp.read(nmea_U, BufferLength);
    unsigned char b = nmea_U[0];
    nmea_U[len] = 0;
    // nmea_UpacketSize = packetSize;
    //Serial.print(nmea_U);
    //line_U = true;
    return true;
  }  // udp PACKET DEALT WITH
     // }
  return false;
}

IPAddress Get_UDP_IP(IPAddress ip, IPAddress mk) {
  uint16_t ip_h = ((ip[0] << 8) | ip[1]);  // high 2 bytes
  uint16_t ip_l = ((ip[2] << 8) | ip[3]);  // low 2 bytes
  uint16_t mk_h = ((mk[0] << 8) | mk[1]);  // high 2 bytes
  uint16_t mk_l = ((mk[2] << 8) | mk[3]);  // low 2 bytes
  // reverse the mask
  mk_h = ~mk_h;
  mk_l = ~mk_l;
  // bitwise OR the net IP with the reversed mask
  ip_h = ip_h | mk_h;
  ip_l = ip_l | mk_l;
  // ip to return the result
  ip[0] = highByte(ip_h);
  ip[1] = lowByte(ip_h);
  ip[2] = highByte(ip_l);
  ip[3] = lowByte(ip_l);
  return ip;
}

void UDPSEND(const char* buf) {                              // this is the one that saves repetitious code!
  Udp.beginPacket(udp_st, atoi(Current_Settings.UDP_PORT));  //if connected and 'AP and STA' else use alternate udp_AP?
  Udp.print(buf);
  Udp.endPacket();

  // Udp.beginPacket(udp_ap, udpport);
  // Udp.print(buf);
  // Udp.endPacket();
}

//************ Music stuff  Purely to explore if the ESP32 has any spare capacity while doing display etc!
//***

//*****   AUDIO ****  STRICTLY experimental - needs three resistors moving to wire in the Is2 audio chip!
void Audio_setup() {
  if (!hasSD) {
    Serial.println("Audio setup FAILED - no SD");
    return;
  }
  Serial.println("Audio setup");
  delay(200);
  audio.setPinout(I2S_BCLK, I2S_LRCK, I2S_DOUT);
  audio.setVolume(15);  // 0...21
  if (audio.connecttoFS(SD, "/StartSound.mp3")) {
    delay(10);
    if (audio.isRunning()) { Serial.println("StartSound.mp3"); }
    while (audio.isRunning()) {
      audio.loop();
      vTaskDelay(1);
    }
  } else {
    gfx->println("No Audio");
    Serial.println("No Audio");
  };
}

struct Music_info {
  String name;
  int length;
  int runtime;
  int volume;
  int status;
  int mute_volume;
  int m;
  int s;
} music_info = { "", 0, 0, 0, 0, 0, 0, 0 };

int get_music_list(fs::FS& fs, const char* dirname, uint8_t levels) {  // uses char* File_List[30] ?? how to pass the name here ??
  //Serial.printf("Listing directory: %s\n", dirname);
  bool ismusic;
  int i = 0;
  char temp[20];
  File root = fs.open(dirname);
  if (!root) {
    // Serial.println("Failed to open directory");
    return i;
  }
  if (!root.isDirectory()) {
    //  Serial.println("Not a directory");
    return i;
  }
  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
    } else {
      strcpy(temp, file.name());
      // if (temp.endsWith(".wav")) {
      // Serial.printf("    temp %s  strstr %i \n",temp,strstr(temp,".mp3"));//strstr(temp,".wav"),strstr(temp,".wav"),( strstr(temp,".wav") || strstr(temp,".wav")));
      if (strstr(temp, ".mp3")) {
        strcpy(File_List[i], temp);  //, sizeof(File_List[i]));  // char array version of '=' !! with limit on size
        i++;
      }
      //} else if (temp.endsWith(".mp3")) {
      //strcpy(File_List[i],temp); // sizeof(File_List[i]));
      //  i++;
      //}
    }
    file = root.openNextFile();
  }
  return i;
}


void open_new_song(String dir, String filename) {
  Serial.print(" Open _NEW song..");
  Serial.println(filename);
  //music_info.name = filename.substring(1, filename.indexOf("."));
  //Serial.print(" audio input..");
  //Serial.println(music_info.name);
  String FullName;
  FullName = dir + filename;
  //Serial.print("** Audio selected<");Serial.print(FullName);Serial.println(">");
  audio.connecttoFS(SD, FullName.c_str());
  //music_info.runtime = audio.getAudioCurrentTime();
  //music_info.length = audio.getAudioFileDuration();
  //music_info.volume = audio.getVolume();
  //music_info.status = 1;
  //music_info.m = music_info.length / 60;
  //music_info.s = music_info.length % 60;
}








//************ TIMING FUNCTIONS FOR TESTING PURPOSES ONLY ******************
//Note this is also an example of how useful Function overloading can be!!
void EventTiming(String input) {
  EventTiming(input, 1);  // 1 should be ignored as this is start or stop! but will also give immediate print!
}

void EventTiming(String input, int number) {  // Event timing, Usage START, STOP , 'Descrption text'   Number waits for the Nth call before serial.printing results (Description text + results).
  static unsigned long Start_time;
  static unsigned long timedInterval;
  static unsigned long _MaxInterval;
  static unsigned long SUMTotal;
  static int calls = 0;
  static int reads = 0;
  long NOW = micros();
  if (input == "START") {
    Start_time = NOW;
    return;
  }
  if (input == "STOP") {
    timedInterval = NOW - Start_time;
    SUMTotal = SUMTotal + timedInterval;
    if (timedInterval >= _MaxInterval) { _MaxInterval = timedInterval; }
    reads++;
    return;
  }
  calls++;
  if (calls < number) { return; }
  if (reads >= 2) {

    if (calls >= 2) {
      Serial.print("\r\n TIMING ");
      Serial.print(input);
      Serial.print(" Using (");
      Serial.print(reads);
      Serial.print(") Samples");
      Serial.print(" last: ");
      Serial.print(timedInterval);
      Serial.print("us Average : ");
      Serial.print(SUMTotal / reads);
      Serial.print("us  Max : ");
      Serial.print(_MaxInterval);
      Serial.println("uS");
    } else {
      Serial.print("\r\n TIMED ");
      Serial.print(input);
      Serial.print(" was :");
      Serial.print(timedInterval);
      Serial.println("uS");
    }
    _MaxInterval = 0;
    SUMTotal = 0;
    reads = 0;
    calls = 0;
  }
}

char* LattoString(double data) {
  static char buff[25];
  double pos;
  pos = data;
  int degrees = int(pos);
  float minutes = (pos - degrees) * 60;
  bool direction;
  direction = (pos >= 0);
  snprintf(buff, sizeof(buff), " %2ideg %6.3fmin %s", abs(degrees), abs(minutes), direction ? "N" : "S");

  return buff;
}
char* LongtoString(double data) {
  static char buff[25];
  double pos;
  pos = data;
  int degrees = int(pos);
  float minutes = (pos - degrees) * 60;
  bool direction;
  direction = (pos >= 0);
  snprintf(buff, sizeof(buff), "%3ideg %6.3fmin %s", abs(degrees), abs(minutes), direction ? "E" : "W");

  return buff;
}

String disconnectreason(int reason) {
  switch (reason) {
    case 1: return "UNSPECIFIED"; break;
    case 2: return "AUTH_EXPIRE"; break;
    case 3: return "AUTH_LEAVE"; break;
    case 4: return "ASSOC_EXPIRE"; break;
    case 5: return "ASSOC_TOOMANY"; break;
    case 6: return "NOT_AUTHED"; break;
    case 7: return "NOT_ASSOCED"; break;
    case 8: return "ASSOC_LEAVE"; break;
    case 9: return "ASSOC_NOT_AUTHED"; break;
    case 10: return "DISASSOC_PWRCAP_BAD"; break;
    case 11: return "DISASSOC_SUPCHAN_BAD"; break;
    case 13: return "IE_INVALID"; break;
    case 14: return "MIC_FAILURE"; break;
    case 15: return "4WAY_HANDSHAKE_TIMEOUT"; break;
    case 16: return "GROUP_KEY_UPDATE_TIMEOUT"; break;
    case 17: return "IE_IN_4WAY_DIFFERS"; break;
    case 18: return "GROUP_CIPHER_INVALID"; break;
    case 19: return "PAIRWISE_CIPHER_INVALID"; break;
    case 20: return "AKMP_INVALID"; break;
    case 21: return "UNSUPP_RSN_IE_VERSION"; break;
    case 22: return "INVALID_RSN_IE_CAP"; break;
    case 23: return "802_1X_AUTH_FAILED"; break;
    case 24: return "CIPHER_SUITE_REJECTED"; break;
    case 200: return "BEACON_TIMEOUT"; break;
    case 201: return "NO_AP_FOUND"; break;
    case 202: return "AUTH_FAIL"; break;
    case 203: return "ASSOC_FAIL"; break;
    case 204: return "HANDSHAKE_TIMEOUT"; break;
    default: return "Unknown"; break;
  }
  return "Unknown";
}

void WiFiEventPrint(WiFiEvent_t event) {
  switch (event) {
    case ARDUINO_EVENT_WIFI_READY:
      Serial.println("   00 ESP32 WiFi ready");
      break;
    case ARDUINO_EVENT_WIFI_SCAN_DONE:
      Serial.println("   01 ESP32 finish scanning AP");
      break;
    case ARDUINO_EVENT_WIFI_STA_START:
      Serial.println("   02 ESP32 station start");
      break;
    case ARDUINO_EVENT_WIFI_STA_STOP:
      Serial.println("   03 ESP32 station stop");
      break;
    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
      Serial.println("   04 ESP32 station connected to AP");
      break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      Serial.println("   05 ESP32 station disconnected from AP");
      break;
    case ARDUINO_EVENT_WIFI_STA_AUTHMODE_CHANGE:
      Serial.println("   06 the auth mode of AP connected by ESP32 station changed");
      break;
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      Serial.println("   07 ESP32 station got IP from connected AP");
      break;
    case ARDUINO_EVENT_WIFI_STA_GOT_IP6:
      Serial.println("   08 ESP32 station interface v6IP addr is preferred");
      break;
    case ARDUINO_EVENT_WIFI_STA_LOST_IP:
      Serial.println("   09 ESP32 station lost IP and the IP is reset to 0");
      break;
    case ARDUINO_EVENT_WIFI_AP_START:
      Serial.println("   10 ESP32 soft-AP start");
      break;
    case ARDUINO_EVENT_WIFI_AP_STOP:
      Serial.println("   11 ESP32 soft-AP stop");
      break;
    case ARDUINO_EVENT_WIFI_AP_STACONNECTED:
      Serial.println("   12 a station connected to ESP32 soft-AP");
      //StationsConnectedtomyAP = StationsConnectedtomyAP + 1;
      break;
    case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
      Serial.println("   13 a station disconnected from ESP32 soft-AP");
      //StationsConnectedtomyAP = StationsConnectedtomyAP - 1;
      //if (StationsConnectedtomyAP == 0) {}
      break;
    case ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED:
      Serial.println("   14 ESP32 soft-AP assign an IP to a connected station");
      break;
    case ARDUINO_EVENT_WIFI_AP_PROBEREQRECVED:
      Serial.println("   15 Receive probe request packet in soft-AP interface");
      break;
    case ARDUINO_EVENT_WIFI_AP_GOT_IP6:
      Serial.println("   16 ESP32 ap interface v6IP addr is preferred");
      break;
    case ARDUINO_EVENT_WIFI_FTM_REPORT:
      Serial.println("   17 fine time measurement report");
      break;
    case ARDUINO_EVENT_ETH_START:
      Serial.println("   18 ESP32 ethernet start");
      break;
    case ARDUINO_EVENT_ETH_STOP:
      Serial.println("   19 ESP32 ethernet stop");
      break;
    case ARDUINO_EVENT_ETH_CONNECTED:
      Serial.println("   20 ESP32 ethernet phy link up");
      break;
    case ARDUINO_EVENT_ETH_DISCONNECTED:
      Serial.println("   21 ESP32 ethernet phy link down");
      break;
    case ARDUINO_EVENT_ETH_GOT_IP:
      Serial.println("   22 ESP32 ethernet got IP from connected AP");
      break;
    case ARDUINO_EVENT_ETH_GOT_IP6:
      Serial.println("   23 ESP32 ethernet interface v6IP addr is preferred");
      break;
    case ARDUINO_EVENT_WPS_ER_SUCCESS:
      Serial.println("   24 ESP32 station wps succeeds in enrollee mode");
      break;
    case ARDUINO_EVENT_WPS_ER_FAILED:
      Serial.println("   25 ESP32 station wps fails in enrollee mode");
      break;
    case ARDUINO_EVENT_WPS_ER_TIMEOUT:
      Serial.println("   26 ESP32 station wps timeout in enrollee mode");
      break;
    case ARDUINO_EVENT_WPS_ER_PIN:
      Serial.println("   27 ESP32 station wps pin code in enrollee mode");
      break;
    default:
      break;
  }
}

