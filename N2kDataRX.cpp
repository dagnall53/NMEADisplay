/*
N2KdataRX.cpp
uses functions from :
Copyright (c) 2015-2018 Timo Lappalainen, Kave Oy, www.kave.fi
Adding AIS (c) 2019 Ronnie Zeiller, www.zeiller.eu

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to use,
copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the
Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


my work here is based on example in Examples  N2KdataRX.cpp
*/

#include <N2kMessages.h>
#include <N2kTypes.h>
#include <math.h>
#include <string.h>

const double radToDeg = 180.0 / M_PI;
#include "aux_functions.h"
#include "Structures.h"
#include "N2kDataRX.h"
extern _sBoatData BoatData;  // BoatData values for the display , int double , when read, when displayed etc 


  double Latitude;
  double Longitude;
  double Altitude;
  //held in boatdata double Variation;
  double Heading;
  double TargetHeading;
  double COG;
  double MCOG;
  double SOG;
  double WindSpeed;
  double WindAngle;
  bool WindSourceApparent;
  double RudderPosition;
  uint16_t DaysSince1970;
  double SecondsSinceMidnight;
  tNMEA0183 *pNMEA0183;
  

//*****************************************************************************
void HandleHeading(const tN2kMsg &N2kMsg) { //127250
  /*
  1 Sequence ID
  2 Heading Sensor Reading
  3 Deviation
  4 Variation
  5 Heading Sensor Reference
  6 NMEA Reserved
  {"Vessel Heading", https://github.com/canboat/canboat/blob/master/analyzer/pgn.h
     127250,
     PACKET_COMPLETE,
     PACKET_SINGLE,
     {UINT8_FIELD("SID"),
      ANGLE_U16_FIELD("Heading", NULL),
      ANGLE_I16_FIELD("Deviation", NULL),
      ANGLE_I16_FIELD("Variation", NULL),
      LOOKUP_FIELD("Reference", 2, DIRECTION_REFERENCE),
      RESERVED_FIELD(6),
      END_OF_FIELDS},
     .interval = 100}
     https://canboat.github.io/canboat/canboat.html#pgn-list So gets..  0SID,1Heading,2Deviation,3Variation,4 Dirref,5Reserved, 
     ParseN2kPGN127250  sets index=0 and gets SID,Heading,Deviation,Variation,refref=(tN2kHeadingReference)
  */
  unsigned char SID;
  tN2kHeadingReference ref;
  double Deviation = NMEA0183DoubleNA;  // not used in other places ?
  double _Deviation = 0;
  double _Variation;

  bool SendHDM = true;
  if (ParseN2kHeading(N2kMsg, SID, Heading, _Deviation, _Variation, ref)) {
    if (ref == N2khr_magnetic) {
      if (!N2kIsNA(_Deviation)) {
        Deviation = _Deviation;
        SendHDM = false;
      }  // Update Deviation, send HDG
      if (!N2kIsNA(_Variation)) {
        BoatData.Variation = _Variation;
        SendHDM = false;
      }  // Update Variation, Send HDG
      if (!N2kIsNA(Heading) && !N2kIsNA(_Deviation)) { Heading -= Deviation; }
      if (!N2kIsNA(Heading) && !N2kIsNA(_Variation)) { Heading -= BoatData.Variation; }
      toNewStruct(Heading, BoatData.MagHeading);
       } else {  // data was "true" so send as true
      toNewStruct(Heading, BoatData.TrueHeading);
    }
  }
}



//***************************************

// //*********************  N2000 PILOT FUNCTIONS ***************************
// // *********  Define a Structure for waypoints so we can index them in PGN 129285 - we only use 2 - but it is possible we may get more.
// //
// struct tWAYPOINT {
//   uint16_t ID;
//   char Name[20];
//   double latitude;
//   double longitude;
// };

// /*
// double DistanceToWaypoint,ETATime,BearingOriginToDestinationWaypoint,BearingPositionToDestinationWaypoint, DestinationLatitude,DestinationLongitude,WaypointClosingVelocity;
//   int16_t ETADate;
//   uint32_t OriginWaypoint_index,DestinationWaypoint_index; 
//   bool PerpendicularCrossed,ArrivalCircleEntered;
//   tN2kHeadingReference BearingReference;
//   tN2kDistanceCalculationType CalculationType;
// */


// struct tROUTE {
//   uint16_t StartRPS;
//   uint16_t nItems;
//   int16_t DatabaseID;
//   uint16_t RouteID;
//   uint8_t suppData;
//   char Name[20];
//   uint32_t OriginWaypoint_index;
//   uint32_t DestinationWaypoint_index;
//   tN2kHeadingReference BearingReference;
//   tN2kDistanceCalculationType CalculationType;
// };

// tWAYPOINT Waypoint[5];
// tROUTE MyRoute;


//*****************************************************************************
// void HandleVariation(const tN2kMsg &N2kMsg) {
//   unsigned char SID;
//   tN2kMagneticVariation Source;
//   uint16_t LOCALDaysSince1970;
//   // Just saves the Variation for use in other functions.
//   ParseN2kMagneticVariation(N2kMsg, SID, Source, LOCALDaysSince1970, _Variation);
//   BoatData.Variation = _Variation; // just save value, not sInstData so not need to save time of data etc.. 
//   }


//*****************************************************************************
void HandleBoatSpeed(const tN2kMsg &N2kMsg) { //128259
  unsigned char SID;
  double WaterReferenced;
  double GroundReferenced;
  tN2kSpeedWaterReferenceType SWRT; // water speed reference type 
  // ignore ground referenced! 
  if (ParseN2kBoatSpeed(N2kMsg, SID, WaterReferenced, GroundReferenced, SWRT)) {
     toNewStruct(WaterReferenced, BoatData.STW);
    
  }
}

//*****************************************************************************
void HandleDepth(const tN2kMsg &N2kMsg) {
  unsigned char SID;
  double DepthBelowTransducer;
  double Offset;
  double Range;
  if (ParseN2kWaterDepth(N2kMsg, SID, DepthBelowTransducer, Offset, Range)) {
    toNewStruct(DepthBelowTransducer, BoatData.WaterDepth);
     }
}

void WaterDepth(const tN2kMsg &N2kMsg) { // original name in datatdisplay example
  unsigned char SID;
  double DepthBelowTransducer;
  double Offset;
  double Range;
  if (ParseN2kWaterDepth(N2kMsg, SID, DepthBelowTransducer, Offset, Range)) {
    toNewStruct(DepthBelowTransducer, BoatData.WaterDepth);
     }
}

//*****************************************************************************
void HandlePosition(const tN2kMsg &N2kMsg) {

  if (ParseN2kPGN129025(N2kMsg, Latitude, Longitude)) {
  // Serial.print(" In HandlePosition lat:");
  // Serial.print(Latitude);
  // Serial.print(" Lon:");
  // Serial.println(Longitude);
// needs toNewStruct ??
      // BoatData.Latitude.data = Latitude*1e-07;   // N2000 lat is double at 1e-7 resolution
      // BoatData.Longitude.data = Longitude*1e-07;
      if (Latitude != -1000000000) {
    toNewStruct( Longitude,BoatData.Longitude);
    toNewStruct( Latitude,BoatData.Latitude);}
  }
  
}

double Days_to_GPSdate(int days_since_1970) { // written largely by copilot AI!
  time_t seconds = (time_t)days_since_1970 * 86400; // Convert days to seconds
  struct tm *date = gmtime(&seconds); // Convert to UTC date
  int dd = date->tm_mday;
  int mm = date->tm_mon + 1; // tm_mon is 0-based
  int yy = date->tm_year % 100; // Get last two digits of year
  double ddmmyy = dd * 10000 + mm * 100 + yy;
return ddmmyy;
}


void HandleGNSS(const tN2kMsg &N2kMsg) {
  unsigned char SID;
  tN2kGNSStype GNSStype;
  tN2kGNSSmethod GNSSmethod;
  unsigned char nSatellites;
  double HDOP;
  double PDOP;
  double GeoidalSeparation;
  unsigned char nReferenceStations;
  tN2kGNSStype ReferenceStationType;
  uint16_t ReferenceSationID;
  double AgeOfCorrection;
  if (ParseN2kGNSS(N2kMsg, SID, DaysSince1970, SecondsSinceMidnight, Latitude, Longitude, Altitude, GNSStype, GNSSmethod,
                   nSatellites, HDOP, PDOP, GeoidalSeparation,
                   nReferenceStations, ReferenceStationType, ReferenceSationID, AgeOfCorrection)) {
    if (Latitude != -1000000000) {
    toNewStruct( Longitude,BoatData.Longitude);
    toNewStruct( Latitude,BoatData.Latitude);
    BoatData.SatsInView=NMEA0183DoubleNA; if (nSatellites){BoatData.SatsInView=nSatellites;}
    BoatData.GPSDate = Days_to_GPSdate(DaysSince1970);
    BoatData.GPSTime =SecondsSinceMidnight ;}
  }
}

//*****************************************************************************
void HandleCOGSOG(const tN2kMsg &N2kMsg) {
  unsigned char SID;
  tN2kHeadingReference HeadingReference;


  if (ParseN2kCOGSOGRapid(N2kMsg, SID, HeadingReference, COG, SOG)) {
    //get / set  MCOG
    MCOG = (!N2kIsNA(COG) && !N2kIsNA(BoatData.Variation) ? COG - BoatData.Variation : NMEA0183DoubleNA);
    if (HeadingReference == N2khr_magnetic) {
      MCOG = COG;
      if (!N2kIsNA(BoatData.Variation)) COG -= BoatData.Variation;
    }
    toNewStruct(COG, BoatData.COG);
    toNewStruct(SOG, BoatData.SOG);
    
  }
}

//*****************************************************************************


void HandleGNSSSystemTime(const tN2kMsg &N2kMsg) {
  unsigned char SID;
  uint16_t SystemDate;
  double SystemTime;
  tN2kTimeSource TimeSource;

  if (ParseN2kSystemTime(N2kMsg, SID, SystemDate, SystemTime, TimeSource)) {
    time_t t = tNMEA0183Msg::daysToTime_t(SystemDate);
    tmElements_t tm;
    tNMEA0183Msg::breakTime(t, tm);
    int GPSDay = tNMEA0183Msg::GetDay(tm);
    int GPSMonth = tNMEA0183Msg::GetMonth(tm);
    int GPSYear = tNMEA0183Msg::GetYear(tm);
    int LZD = 0;
    int LZMD = 0;
    //toNewStruct(SystemDate, BoatData.GPSDate);
    // needs setStruct
  }
}



//*****************************************************************************
void HandleWind(const tN2kMsg &N2kMsg) {
  /* see C:\Users\admin\Documents\Arduino\libraries\NMEA2000\Examples\NMEA2000ToNMEA0183\N2KdataRX.cpp
  */
  unsigned char SID;
  tN2kWindReference WindReference;                               // NOTE this is local and N2K and different from the tNMEA0183WindReference
                                                                 //               that we store as (int)   00 ground 01ground north ref 02apparent waterlin 03 theoretical using cog sog 04 centerline theoretical based on water speed
                                                                 // 00=T & 01=T 02,03,04=A
  tNMEA0183WindReference NMEA0183Reference = NMEA0183Wind_Apparent;  // also LOCAL! just true and apparent

  if (ParseN2kWindSpeed(N2kMsg, SID, WindSpeed, WindAngle, WindReference)) {
    WindSourceApparent = false;  // this is the variable we will save for elsewhere
    if (WindReference == N2kWind_Apparent) {
      WindSourceApparent = true;
      NMEA0183Reference = NMEA0183Wind_Apparent;
    }
    // DO we need to check for N2KWind_
    //  uncomment to send immediately
    if (WindSourceApparent) {
      NMEA0183Reference = NMEA0183Wind_Apparent;
    } else {
      NMEA0183Reference = NMEA0183Wind_True;
    };
    toNewStruct(WindAngle * radToDeg, BoatData.WindAngleApp);
    toNewStruct(WindSpeed *1.9438, BoatData.WindSpeedK);
    //
  }
}



//*****************************************************************************
void HandleRudder(const tN2kMsg &N2kMsg) {

  unsigned char Instance;
  tN2kRudderDirectionOrder RudderDirectionOrder;
  double AngleOrder;

  if (ParseN2kRudder(N2kMsg, RudderPosition, Instance, RudderDirectionOrder, AngleOrder)) {
  //set  a rudder position structure? 

  }
}

//---------other utils etc..

String PGNDecode(int PGN) {  // decode the PGN to a readable name.. Useful for the decodeMode the bus?
  // https://endige.com/2050/nmea-2000-pgns-deciphered/
  // see also https://canboat.github.io/canboat/canboat.xml#pgn-list
  // Changed Type to String: from Char*// to avoid the warnings!
  // I have added  to those PGN that store data for timed use: RMB APB RMC etc
  switch (PGN) {
    case 65359: return "Seatalk: Pilot Heading"; break;  //https://github.com/canboat/canboat/blob/master/analyzer/pgn.h
    case 65379: return "Seatalk: Pilot Mode"; break;
    case 65360: return "Seatalk: Pilot Locked Heading"; break;
    case 65311: return "Magnetic Variation (Raymarine Proprietary)"; break;
    case 126720: return "Raymarine Device ID"; break;
    case 126992: return "System Time"; break;
    case 126993: return "Heartbeat"; break;
    case 127237: return "Heading/Track Control"; break;
    case 127245: return "Rudder"; break;
    case 127250: return "Vessel Heading, Deviation, Variation"; break;
    case 127251: return "Rate of Turn"; break;
    case 127258: return "Magnetic Var"; break;
    case 127488: return "Engine Parameters, Rapid Update"; break;
    case 127508: return "Battery Status"; break;
    case 127513: return "Battery Configuration Status"; break;
    case 128259: return "Speed, Water"; break;
    case 128267: return "Water Depth"; break;
    case 128275: return "Distance Log"; break;
    case 129025: return "Position, Rapid Update"; break;
    case 129026: return "COG SOG, Rapid Update"; break;
    case 129029: return "GNSS Position"; break;
    case 129033: return "Local Time Offset"; break;
    case 129044: return "Datum"; break;
    case 129283: return "Cross Track Error"; break;
    case 129284: return "Navigation Data"; break;
    case 129285: return "Navigation — Route/WP information"; break;
    case 129291: return "Set & Drift, Rapid Update"; break;
    case 129539: return "GNSS DOPs"; break;
    case 129540: return "GNSS Sats in View"; break;
    case 130066: return "Route/WP— List Attributes"; break;
    case 130067: return "Route — WP Name & Position"; break;
    case 130074: return "WP List — WP Name & Position"; break;
    case 130306: return "Wind Data"; break;
    case 130310: return "Environmental Parameters-deprecated"; break;
    case 130311: return "Environmental Parameters-deprecated"; break;
    case 130312: return "Temperature"; break;
    case 130313: return "Humidity"; break;
    case 130314: return "Actual Pressure"; break;
    case 130316: return "Temperature, Extended Range"; break;
    case 129038: return "AIS Class A Position Report"; break;
    case 129039: return "AIS Class B Position Report"; break;
    case 129040: return "AIS Class B Extended Position Report"; break;
    case 129041: return "AIS Aids to Navigation (AtoN) Report"; break;
    case 129793: return "AIS UTC and Date Report"; break;
    case 129794: return "AIS Class A Static and Voyage Related Data"; break;
    case 129798: return "AIS SAR Aircraft Position Report"; break;
    case 129809: return "AIS Class B “CS” Static Data Report, Part A"; break;
    case 129810: return "AIS Class B “CS” Static Data Report, Part B"; break;
    case 60928: return "Address Claimed/cannot Claim"; break;
    case 130916: return "Seatalk AP Unknown?"; break;
    case 65240: return "Commanded Address"; break;
    case 127257:return "Attitude yaw pitch etc"; break;

    case 130848:return "Mfr proprietary fast packet";break;
    case 130918:return "Mfr proprietary fast packet";break;
    case 130577:return "Direction Data";break;
    case 126996:return "Product Information";break;

    default: return "Unknown ";break;
  }
  return "unknown";
}


// experimental  Show manufacturer data etc? // ai helped write

bool ParseN2kPGN60928(const tN2kMsg &N2kMsg, uint64_t &NAME) {
  if (N2kMsg.DataLen < 8) return false;

  NAME = 0;
  for (int i = 0; i < 8; i++) {
    NAME |= ((uint64_t)N2kMsg.Data[i]) << (8 * i);
  }
  return true;
}

void HandleMFRData(const tN2kMsg &N2kMsg) {
  //Serial.println("*********parse MFR data 126996 or 60928 *********");

  if (N2kMsg.PGN == 60928) {
    uint64_t NAME = 0;
    if (ParseN2kPGN60928(N2kMsg, NAME)) {
      // Serial.println("PGN 60928 - ISO Address Claim:");
      // Serial.print("  Source Address: "); Serial.println(Source);
      // Optional: decode fields from NAME
      uint8_t industryGroup = (NAME >> 60) & 0x07;
      uint8_t deviceClass    = (NAME >> 56) & 0x0F;
      uint8_t deviceFunction = (NAME >> 48) & 0xFF;
      uint16_t manufacturer  = (NAME >> 21) & 0x7FF;
      uint32_t uniqueID      = NAME & 0x1FFFFF;

      //Serial.print("  Industry Group: "); Serial.println(industryGroup);
      Serial.print("  Device Source:   "); Serial.println(N2kMsg.Source);
      Serial.print("  Device Class:   "); Serial.println(deviceClass);
      Serial.print("  Function Code:  "); Serial.println(deviceFunction);
      Serial.print("  Manufacturer:   "); Serial.println(manufacturer);
      Serial.print("  Unique ID:      "); Serial.println(uniqueID);
      //RequestProductInformation(N2kMsg.Source);
    } else {
      Serial.println("Failed to parse PGN 60928");
    }
  }
  if (N2kMsg.PGN == 126996) {
    // Scalars passed by reference
    unsigned short N2kVersion = 0;
    unsigned short ProductCode = 0;
    unsigned char CertificationLevel = 0;
    unsigned char LoadEquivalency = 0;

    // Buffers for strings
    const int ModelIDSize = 33;
    const int SwCodeSize = 33;
    const int ModelVersionSize = 33;
    const int ModelSerialCodeSize = 33;

    char ModelID[ModelIDSize] = {0};
    char SwCode[SwCodeSize] = {0};
    char ModelVersion[ModelVersionSize] = {0};
    char ModelSerialCode[ModelSerialCodeSize] = {0};

    // Parse the PGN
    if (ParseN2kPGN126996(N2kMsg,
                          N2kVersion,
                          ProductCode,
                          ModelIDSize, ModelID,
                          SwCodeSize, SwCode,
                          ModelVersionSize, ModelVersion,
                          ModelSerialCodeSize, ModelSerialCode,
                          CertificationLevel,
                          LoadEquivalency)) {

      Serial.println("PGN 126996 - Product Information:");
      Serial.print("  NMEA 2000 Version: "); Serial.println(N2kVersion);
      Serial.print("  Product Code: "); Serial.println(ProductCode);
      Serial.print("  Model ID: "); Serial.println(ModelID);
      Serial.print("  Software Code: "); Serial.println(SwCode);
      Serial.print("  Model Version: "); Serial.println(ModelVersion);
      Serial.print("  Serial Code: "); Serial.println(ModelSerialCode);
      Serial.print("  Certification Level: "); Serial.println(CertificationLevel);
      Serial.print("  Load Equivalency: "); Serial.println(LoadEquivalency);
    } else {
      Serial.println("Failed to parse PGN 126996");
    }
  }
}

extern tNMEA2000 &NMEA2000;

void RequestProductInformation(uint8_t destination) {
  tN2kMsg N2kMsg;
  SetN2kPGNISORequest(N2kMsg,0xFF, 126996);  // Request PGN 126996
  //N2kMsg.Destination = destination;    // Use 0xFF for broadcast or specific address
  NMEA2000.SendMsg(N2kMsg);
}


