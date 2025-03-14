/* 
tBoatdata from Timo Llapinen by Dr.András Szép under GNU General Public License (GPL).


*/
#ifndef _BoatData_H_
#define _BoatData_H_


struct MySettings {  //key,ssid,PW,udpport, UDP,serial,Espnow
  int EpromKEY;      // Key is changed to allow check for clean EEprom and no data stored change in the default will result in eeprom being reset
//  int DisplayPage;   // start page after defaults 
  char ssid[16];
  char password[16];
  char UDP_PORT[5];  // hold udp port as char to make using keyboard easier for now. use atoi when needed!!
  bool UDP_ON;
  bool Serial_on;
  bool ESP_NOW_ON;
};
struct MyColors {  // for later Day/Night settings
  uint16_t TextColor;
  uint16_t BackColor;
  uint16_t BorderColor;
};

struct instData{  // struct to hold instrument data AND the time it was updated. 
  double data;
  unsigned long updated;
  bool displayed;
  bool greyed;
};

  struct tBoatData {
    unsigned long DaysSince1970;  // Days since 1970-01-01
    instData SOG,STW,COG,MagHeading,TrueHeading,WaterDepth,SatsInView,
    WindDirectionT, WindDirectionM, WindSpeedK, WindSpeedM,WindAngle,
    WindAngleA
    ;      //instData will be used with NEWUPdate and greys if old

    double Variation,GPSTime,Latitude, Longitude,   // keep GPS stuff in double.. 
     Altitude, HDOP, GeoidalSeparation, DGPSAge,
      WaterTemperature, Offset,RPM;

    int GPSQualityIndicator, SatelliteCount, DGPSReferenceStationID;
    bool MOBActivated;
    char Status;
  };

  struct Button {
  int h, v, width, height, bordersize;
  uint16_t backcol, textcol, BorderColor;
  int Font;                  //-1 == not forced (not used?)
  bool Keypressed;           //used by keypressed
  unsigned long LastDetect;  //used by keypressed
  int PrintLine;      // used for UpdateLinef() 
};

struct Phv{
  int h,v;
};

#endif  // _BoatData_H_
