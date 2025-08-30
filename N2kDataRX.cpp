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
  double Variation;
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
        Variation = _Variation;
        SendHDM = false;
      }  // Update Variation, Send HDG
      if (!N2kIsNA(Heading) && !N2kIsNA(_Deviation)) { Heading -= Deviation; }
      if (!N2kIsNA(Heading) && !N2kIsNA(_Variation)) { Heading -= Variation; }
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
//   ParseN2kMagneticVariation(N2kMsg, SID, Source, LOCALDaysSince1970, Variation);
//   BoatData.Variation = Variation; // just save value, not sInstData so not need to save time of data etc.. 
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
    MCOG = (!N2kIsNA(COG) && !N2kIsNA(Variation) ? COG - Variation : NMEA0183DoubleNA);
    if (HeadingReference == N2khr_magnetic) {
      MCOG = COG;
      if (!N2kIsNA(Variation)) COG -= Variation;
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






