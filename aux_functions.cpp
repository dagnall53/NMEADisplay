/* 
Original version by Dr.András Szép under GNU General Public License (GPL).
but highly modified! 
*/
#include <Arduino.h>  //necessary for the String variables
#include "Structures.h"  // to tell it about the boat data structs.

extern char* pTOKEN;  // const ?global? pointer of type char, used to get the fields in a nmea sentence
               // when a new sentence is processed we advance this pointer n positions 
               // after the beginning of the sentence such that it points to the 1st field.
               // used by function void FillToken(char * ptr) in 

char Field[20][18];    // for the extracted NMEA fields 

#define MAX_NMEA0183_MSG_BUF_LEN 4096
#define MAX_NMEA_FIELDS  64
char nmeaLine[MAX_NMEA0183_MSG_BUF_LEN]; //NMEA0183 message buffer
size_t i=0, j=1;                          //indexers
uint8_t *pointer_to_int;                  //pointer to void *data (!)
int noOfFields = MAX_NMEA_FIELDS;  //max number of NMEA0183 fields
int Num_Conversions,Num_DataFields; //May be needed in sub functions?

int TokenCount(char *ptr) {
  // to count number of  commas in the char array (counts number of data fields present)
  char ch = ',';
  int count = 0;
  for (int i = 0; i < strlen(ptr); i++) { if (ptr[i] == ch) count++; }
  return count;
}

boolean FillToken(char * ptr) {            // 
  /* the function receives a pointer to the char array where the field is to be stored.
   * It uses the global pointer pTOKEN that points to the starting of the field that we
   * want to extract. So we search for the next field delimiter "," which will be pointed
   * by p2 and we get the lenght of the field in the variable len. We copy len characters
   * to the destination char array and terminate the array with a 0 (ZERO). So if a field
   * is empty, len = 0 and ptr[0]=0. This can be used, later, to test if the field was empty.
   * Finally as p2 is pointing to a "," we increment it by 1 and copy it to pTOKEN so that
   * pTOKEN will be pointing to the starting of the next field.
   */
  char *p2 = strchr(pTOKEN, ',');
  if (p2 == NULL) { return false; }
  int len = p2 - pTOKEN;
  memcpy(ptr, pTOKEN, len);
  ptr[len] = 0;
  pTOKEN = p2 + 1;
  return true;
}

boolean FillTokenLast(char * ptr) {
  // All NMEA messages should have the checksum, with a * delimeter for the last data field. -- but some coders may forget this and just end with CR..(!)
  // This is the same basic code as the FillToken code Luis wrote, but searches for '*' and if not found, looks for a CR.
  // It is ONLY used to extract the last expected datafield from NMEA messages.  - And we counted the number of data fields using a ", count" in  TokenCount.
  // Therefore this will extract the last datafield from NMEA data messages with either  a * or a CR  delimiting the last datafield. 
  // could probably be written in a more elegant way.. 
  char *p2 = strchr(pTOKEN, '*');
  char *p3 = strchr(pTOKEN, 0x0D);
  if ((p2 == NULL) && (p3 == NULL)) { return false; }
  if (p2 == NULL) {
    int len = p3 - pTOKEN;     // Second choice, "*" was not found/missing so just extract ALL the remaining data before the CR. 
    memcpy(ptr, pTOKEN, len);
    ptr[len] = 0;
    pTOKEN = p3 + 1;                 // not a lot of point in this as this was the end of the message.. ?                    
  }
  else {                             // not yet at end of message! We found a "*" 
    int len = p2 - pTOKEN;           // get length from last comma to "*" character
    memcpy(ptr, pTOKEN, len);        // copy whatever there is to ptr
    ptr[len] = 0;                      // place the end mark (0) 
    pTOKEN = p2 + 1;                 // could probably remove this as this subroutine is only called for the last data field. ? 
  }
  return true;
}


bool processPacket(const char* buf, tBoatData &BoatData) {// reads char array buf and places (updates) data if found in stringND 
  char *p;
  //STMissed="";  // reset  omitted message debug message string
  Num_Conversions = 0;
  Num_DataFields = TokenCount(pTOKEN);                   //  TokenCount reads the number of commas in the pointed nmea_X[] buffer                                        
  pTOKEN = pTOKEN + 1;                                   // pTOKEN points advances $/! 
  for (int n = 0; n <= Num_DataFields - 1; n++) {
    p = Field[n];
    if (!FillToken(Field[n])) { return false; }        // FillToken looks for "," places the data pointed to into Field[n] 
  }
  p = Field[Num_DataFields];                             // searches for '*' and if not found, looks for a CR
  if (!FillTokenLast(Field[Num_DataFields])) { return false; }
  char Command[8];
  Command[0]=Field[0][2];Command[1]=Field[0][3];Command[2]=Field[0][4];Command[3]=0 ;Command[4]=0;
  Serial.printf("  and now NMEA <%s> to be processed \n",Command); 
  Serial.printf("  Found  <%i> Fields Field1<%s> Field2<%s> Field3<%s>\n",Num_DataFields,Field[1],Field[2],Field[3]); 
  
  if (strcmp(Command,"DBT") == 0) {
    Serial.printf(" in  <%i> Fields Field1<%s> Field2<%s>Field3<%s> \n",Num_DataFields,Field[1],Field[2],Field[3]);
    Serial.printf("  Process NMEA <%s> processed %f from <%s> \n",Command,atof(Field[3]),Field[3]); 
          BoatData.WaterDepth = atof(Field[3]);
  }
          //depth = BoatData.WaterDepth;
//         }else if (Command == "RMC") {    //nav info
//           BoatData.SOG = nmeaStringData[7];
//           BoatData.Latitude = convertGPString(nmeaStringData[3]) + nmeaStringData[4];
//           BoatData.Longitude = convertGPString(nmeaStringData[5]) + nmeaStringData[6];
//           BoatData.UTC = nmeaStringData[1];
//           // int hours = BoatData.UTC.substring(0, 2).toInt();
//           // int minutes = BoatData.UTC.substring(2, 4).toInt();
//           // int seconds = BoatData.UTC.substring(4, 6).toInt();
//           // BoatData.GPSTime = BoatData.UTC.toDouble();
//           // BoatData.UTC = int2string(hours) + ":" + int2string(minutes) + ":" + int2string(seconds);           
//          // BoatData.Date = "20"+ nmeaStringData[9].substring(4,6) + "-" + nmeaStringData[9].substring(2,4) + "-" + nmeaStringData[9].substring(0,2) ;
 
//         }

// //         if(command == "APB") {
// //  //          Serial.print("APB");    //autopilot
// //         } else if (command == "DBK") {
// //   //          Serial.print("DBK");
// //         } else if (command == "DBT") {
// //           BoatData.WaterDepth = nmeaStringData[3];
// //           depth = BoatData.WaterDepth;
// //           jsonDoc["depth"] = depth; 
// //           notifyClients();
// //         } else if (command == "DPT") {
// //  /*
// //           BoatData.WaterDepth = nmeaStringData[1];
// //           depth = BoatData.WaterDepth;
// //           jsonDoc["depth"] = depth; 
// //           notifyClients(); 
// //  */
// //         } else if (command == "GGA") {
// //           BoatData.UTC = nmeaStringData[1];
// //           int hours = BoatData.UTC.substring(0, 2).toInt();
// //           int minutes = BoatData.UTC.substring(2, 4).toInt();
// //           int seconds = BoatData.UTC.substring(4, 6).toInt();
// //           BoatData.GPSTime = BoatData.UTC.toDouble();
//           BoatData.UTC = int2string(hours) + ":" + int2string(minutes) + ":" + int2string(seconds);
//           timedate = BoatData.UTC;  
//           jsonDoc["timedate"] = timedate; 
//  //          Serial.printf("GGA %s", timedate);      
//           BoatData.Latitude = convertGPString(nmeaStringData[2]) + nmeaStringData[3];
//           latitude = BoatData.Latitude;
//           jsonDoc["latitude"] = latitude; 
//           BoatData.Longitude = convertGPString(nmeaStringData[4]) + nmeaStringData[5];
//           longitude = BoatData.Longitude;
//           jsonDoc["longitude"] = longitude;  
//           notifyClients();         
//         } else if (command == "GLL") {
//           BoatData.Latitude = convertGPString(nmeaStringData[1]) + nmeaStringData[2];
//           latitude = BoatData.Latitude;
//           jsonDoc["latitude"] = latitude;
//           BoatData.Longitude = convertGPString(nmeaStringData[3]) + nmeaStringData[4];
//           longitude = BoatData.Longitude;
//           jsonDoc["longitude"] = longitude;
//           BoatData.UTC = nmeaStringData[5];
//           int hours = BoatData.UTC.substring(0, 2).toInt();
//           int minutes = BoatData.UTC.substring(2, 4).toInt();
//           int seconds = BoatData.UTC.substring(4, 6).toInt();
//           BoatData.UTC = int2string(hours) + ":" + int2string(minutes) + ":" + int2string(seconds);
//           timedate = BoatData.UTC;  
//           jsonDoc["timedate"] = timedate;  
//           notifyClients(); 
//   //          Serial.printf("GLL %s", timedate);         
//         } else if (command == "GSA") { //GPS Sat
//         //
//         } else if (command == "HDG") {
//           BoatData.HeadingM = String( int(nmeaStringData[1].toDouble()));
//           heading = BoatData.HeadingM + "°";  
//           jsonDoc["heading"] = heading;
//           notifyClients();
//         } else if (command == "HDM") {
//           BoatData.HeadingM = String( int(nmeaStringData[1].toDouble()));
//           heading = BoatData.HeadingM + "°";  
//           jsonDoc["heading"] = heading; 
//           notifyClients();
//         } else if (command == "HDT") {
//           BoatData.HeadingT = String( int(nmeaStringData[1].toDouble()));
//           heading = BoatData.HeadingT + "°";  
//           jsonDoc["heading"] = heading; 
//           notifyClients();
//         } else if (command == "MTW") {
//           BoatData.WaterTemperature = nmeaStringData[1];
//           watertemp = BoatData.WaterTemperature + "°C";  
//           jsonDoc["watertemp"] = watertemp; 
//           notifyClients();
//         } else if (command == "MWD") {
//           BoatData.WindDirectionT = String( int(nmeaStringData[1].toDouble()));
//           winddir  = BoatData.WindDirectionT + "°true";  
//           jsonDoc["winddir"] = winddir;
//           BoatData.WindDirectionM = String( int(nmeaStringData[3].toDouble()));
//   //          winddir  = BoatData.WindDirectionM + "m";  
//   //          jsonDoc["winddir"] = winddir;
//           BoatData.WindSpeedK = String( int(nmeaStringData[5].toDouble()));
//           windspeed = BoatData.WindSpeedK;  
//           jsonDoc["windspeed"] = windspeed; 
//           notifyClients();
//         } else if (command == "MWV") { //wind speed and angle
//           BoatData.WindDirectionT = String( int(nmeaStringData[1].toDouble()));
//           winddir = BoatData.WindDirectionT + "°app";  
//           jsonDoc["winddir"] = winddir;
//           BoatData.WindSpeedK = nmeaStringData[3];  
//           windspeed = BoatData.WindSpeedK;  // + nmeaStringData[4];
//           jsonDoc["windspeed"] = windspeed + " app"; 
//           notifyClients();
//         } else if (command == "RMB") {    //nav info
//   //          Serial.print("RMB");        //waypoint info
//         } else if (command == "RMC") {    //nav info
//           BoatData.SOG = nmeaStringData[7];
//           speed = BoatData.SOG;  
//           jsonDoc["speed"] = speed;
//           BoatData.Latitude = convertGPString(nmeaStringData[3]) + nmeaStringData[4];
//           latitude = BoatData.Latitude;
//           jsonDoc["latitude"] = latitude;
//           BoatData.Longitude = convertGPString(nmeaStringData[5]) + nmeaStringData[6];
//           longitude = BoatData.Longitude;
//           jsonDoc["longitude"] = longitude;
//           BoatData.UTC = nmeaStringData[1];
//           int hours = BoatData.UTC.substring(0, 2).toInt();
//           int minutes = BoatData.UTC.substring(2, 4).toInt();
//           int seconds = BoatData.UTC.substring(4, 6).toInt();
//           BoatData.UTC = int2string(hours) + ":" + int2string(minutes) + ":" + int2string(seconds);            
//           BoatData.Date = "20"+ nmeaStringData[9].substring(4,6) + "-" + nmeaStringData[9].substring(2,4) + "-" + nmeaStringData[9].substring(0,2) ;
//           timedate = BoatData.Date + " " + BoatData.UTC;
//           jsonDoc["timedate"] = timedate; 
//           notifyClients(); 
//   //          Serial.printf("RMC %s", timedate);     
//         } else if (command == "RPM") {  //engine RPM
//           if (nmeaStringData[2] == "1") {     //engine no.1
//             BoatData.RPM = String( int (nmeaStringData[3].toDouble()));
//             rpm = BoatData.RPM;  
//             jsonDoc["rpm"] = rpm;
//             notifyClients();
//           }
//         } else if (command == "VBW") {  //dual ground/water speed longitudal/transverse
//         //
//         } else if (command == "VDO") {  
//         //
//         } else if (command == "VDM") {  
//         //
//         } else if (command == "APB") {  
//         //
//         } else if (command == "VHW") {  //speed and Heading over water
//           BoatData.HeadingT = String( int(nmeaStringData[1].toDouble()));
//           heading = BoatData.HeadingT + "t";
//           jsonDoc["heading"] = heading;
//           BoatData.HeadingM = String( int(nmeaStringData[3].toDouble()));
//           heading = BoatData.HeadingM + "m";
//           jsonDoc["heading"] = heading;              
//           BoatData.Speed = nmeaStringData[5];
//           speed = BoatData.Speed;
//           jsonDoc["speed"] = speed;   
//           notifyClients();    
//         //
//         } else if (command == "VTG") {  //Track Made Good and Ground Speed
//         //
//         } else if (command == "ZDA") { //Date&Time
//           BoatData.Date = nmeaStringData[4] + "-" + nmeaStringData[3] + "-" + nmeaStringData[2];
//           BoatData.UTC = nmeaStringData[1];
//           int hours = BoatData.UTC.substring(0, 2).toInt();
//           int minutes = BoatData.UTC.substring(2, 4).toInt();
//           int seconds = BoatData.UTC.substring(4, 6).toInt();
//           BoatData.UTC = int2string(hours) + ":" + int2string(minutes) + ":" + int2string(seconds);
//           timedate = BoatData.Date + " " + BoatData.UTC;
//           jsonDoc["timedate"] = timedate;  
//           notifyClients();
//   //          Serial.printf("ZDA %s", timedate);       
//         } 
//         else {
//           Serial.println("unsupported NMEA0183 sentence");
//         }
//         jsonDoc.clear();
//         j=0;
//       }
return false;
   }


 