/* 
tBoatdata from Timo Llapinen by Dr.András Szép under GNU General Public License (GPL).


*/
#ifndef _BoatData_H_
#define _BoatData_H_
#include <NMEA0183.h>  // for the TL NMEA0183 library functions
#include <NMEA0183Msg.h>
#include <NMEA0183Messages.h> // for the doubleNA

struct JSONCONFIG {  // will be Display_Config for the JSON set Defaults and user settings for displays 
  char Mag_Var[15]; // got to save double variable as a string! east is positive
  int Start_Page ;
  char PanelName[25];
  char APpassword[25]; 
  char FourWayBR[10] ;
  char FourWayBL[10] ; 
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
//  double DataforGraph[202];
  unsigned long updated;
  bool displayed;
  bool greyed;
  bool graphed;
  int source;  // to try an experiment with two GPS to see how they track. 
};



struct tBoatData {
  unsigned long DaysSince1970;  // Days since 1970-01-01

  instData SOG, STW, COG, Latitude, Longitude,MagHeading, TrueHeading, WaterDepth,
    WindDirectionT, WindDirectionM, WindSpeedK, WindSpeedM, WindAngleApp, WindAngleGround;
     //instData will be used with NEWUPdate and greys if old

  double SatsInView,Variation, GPSTime, GPSDate,  // keep some GPS stuff in double ..
    Altitude, HDOP, GeoidalSeparation, DGPSAge;
    
  bool MOBActivated;

};

struct Button {
  int h, v, width, height, bordersize;
  uint16_t BackColor, TextColor, BorderColor;
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
