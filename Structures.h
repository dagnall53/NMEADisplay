/* 
tBoatdata from Timo Llapinen by Dr.András Szép under GNU General Public License (GPL).


*/
#ifndef _BoatData_H_
#define _BoatData_H_
#include <NMEA0183.h>  // for the TL NMEA0183 library functions
#include <NMEA0183Msg.h>
#include <NMEA0183Messages.h> // for the doubleNA

struct JSONCONFIG {  // for the JSON set Defaults and user settings for displays 
  char Mag_Var[15]; // got to save double variable as a string! east is positive
  int Start_Page ;
  char PanelName[25];
  char APpassword[25]; 
   
};


struct MySettings {  // MAINLY WIFI AND DATA LOGGING key,ssid,PW,udpport, UDP,serial,Espnow
  int EpromKEY;      // Key is changed to allow check for clean EEprom and no data stored change in the default will result in eeprom being reset
                     //  int DisplayPage;   // start page after defaults
  char ssid[25];
  char password[25];
  char UDP_PORT[5];  // hold udp port as char to make using keyboard easier for now. use atoi when needed!!
  bool UDP_ON;
  bool Serial_on;
  bool ESP_NOW_ON;
  bool Log_ON;
  bool NMEA_log_ON;
};
struct MyColors {  // for later Day/Night settings
  uint16_t TextColor;
  uint16_t BackColor;
  uint16_t BorderColor;
};

struct instData {  // struct to hold instrument data AND the time it was updated.
  double data = NMEA0183DoubleNA;
  double lastdata = NMEA0183DoubleNA;
  unsigned long updated;
  bool displayed;
  bool greyed;
  bool graphed;
};



struct tBoatData {
  unsigned long DaysSince1970;  // Days since 1970-01-01

  instData SOG, STW, COG, MagHeading, TrueHeading, WaterDepth,
    WindDirectionT, WindDirectionM, WindSpeedK, WindSpeedM, WindAngleApp;
     //instData will be used with NEWUPdate and greys if old

  double SatsInView,Variation, GPSTime, Latitude, Longitude, GPSDate,  // keep GPS stuff in double ..
    Altitude, HDOP, GeoidalSeparation, DGPSAge;
    
//not used .. yet?
  int GPSQualityIndicator, DGPSReferenceStationID;
  bool MOBActivated;

};

struct Button {
  int h, v, width, height, bordersize;
  uint16_t backcol, textcol, BorderColor;
  int Font;                  //-1 == not forced (not used?)
  bool Keypressed;           //used by keypressed
  unsigned long LastDetect;  //used by keypressed
  int PrintLine;             // used for UpdateLinef()
  bool screenfull,debugpause;
};

struct Phv {
  int h, v;
};

#endif  // _BoatData_H_
