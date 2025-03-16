/* 
Original version by Dr.András Szép under GNU General Public License (GPL).
but highly modified! 
*/

#include <Arduino.h>  //necessary for the String variables
#include "aux_functions.h"
// defines gfx and has pin details of the display
#include <NMEA0183.h>  // for the TL NMEA0183 library functions
#include <NMEA0183Msg.h>
#include <NMEA0183Messages.h>

extern int text_offset;
extern int MasterFont;
extern void setFont(int);
extern int text_height;
extern bool debugpause;

extern double NMEA0183GetDouble(const char *data);  // have to do this as its local to NMEA0183Messagesmessages.cpp!

extern char *pTOKEN;  // const ?global? pointer of type char, used to get the fields in a nmea sentence
                      // when a new sentence is processed we advance this pointer n positions
                      // after the beginning of the sentence such that it points to the 1st field.
                      // used by function void FillToken(char * ptr) in

char Field[20][18];  // for the extracted NMEA fields

#define MAX_NMEA_FIELDS 64
char nmeaLine[MAX_NMEA0183_MSG_BUF_LEN];  //NMEA0183 message buffer
size_t i = 0, j = 1;                      //indexers
uint8_t *pointer_to_int;                  //pointer to void *data (!)
int noOfFields = MAX_NMEA_FIELDS;         //max number of NMEA0183 fields
int Num_Conversions, Num_DataFields;      //May be needed in sub functions?

int TokenCount(char *ptr) {
  // to count number of  commas in the char array (counts number of data fields present)
  char ch = ',';
  int count = 0;
  for (int i = 0; i < strlen(ptr); i++) {
    if (ptr[i] == ch) count++;
  }
  return count;
}

boolean FillToken(char *ptr) {  //
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

boolean FillTokenLast(char *ptr) {
  // All NMEA messages should have the checksum, with a * delimeter for the last data field. -- but some coders may forget this and just end with CR..(!)
  // This is the same basic code as the FillToken code Luis wrote, but searches for '*' and if not found, looks for a CR.
  // It is ONLY used to extract the last expected datafield from NMEA messages.  - And we counted the number of data fields using a ", count" in  TokenCount.
  // Therefore this will extract the last datafield from NMEA data messages with either  a * or a CR  delimiting the last datafield.
  // could probably be written in a more elegant way..
  char *p2 = strchr(pTOKEN, '*');
  char *p3 = strchr(pTOKEN, 0x0D);
  if ((p2 == NULL) && (p3 == NULL)) { return false; }
  if (p2 == NULL) {
    int len = p3 - pTOKEN;  // Second choice, "*" was not found/missing so just extract ALL the remaining data before the CR.
    memcpy(ptr, pTOKEN, len);
    ptr[len] = 0;
    pTOKEN = p3 + 1;           // not a lot of point in this as this was the end of the message.. ?
  } else {                     // not yet at end of message! We found a "*"
    int len = p2 - pTOKEN;     // get length from last comma to "*" character
    memcpy(ptr, pTOKEN, len);  // copy whatever there is to ptr
    ptr[len] = 0;              // place the end mark (0)
    pTOKEN = p2 + 1;           // could probably remove this as this subroutine is only called for the last data field. ?
  }
  return true;
}





extern void EventTiming(String input);  // to permit timing functions here during development

bool NeedleinHaystack(char ch1, char ch2, char ch3, char *haystack, int &compareOffset) {
  // Serial.printf("\n Looking for<%c%c%c> in strlen(%i) %s \n", ch1, ch2, ch3, strlen(haystack), haystack);
  compareOffset = 0;
  // if (needle[0] == '\0') { return false; }
  for (compareOffset = 0; (compareOffset <= strlen(haystack)); compareOffset++) {
    if ((ch1 == haystack[compareOffset]) && (ch2 == haystack[compareOffset + 1]) && (ch3 == haystack[compareOffset + 2])) {
      //    Serial.printf("Found at %i\n", compareOffset);
      return true;
    }
  }
  compareOffset = 0;
  return false;
}

//********* Add this if needed in the case statements to help sort bugs!
// Serial.println(" Fields:");
// for (int x = 0; x <= Num_DataFields; x++) {
//   Serial.print(Field[x]);
//   Serial.print(",");
// }
// Serial.println("> ");

void toNewStruct(char *field, instData &data) {
  data.data = atof(field);
  data.updated = millis();
  data.displayed = false;
  data.greyed = false;
}


bool processPacket(const char *buf, tBoatData &BoatData) {  // reads char array buf and places (updates) data if found in stringND
  char *p;
  int Index;
  int Num_Conversions = 0;
  Num_DataFields = TokenCount(pTOKEN);  //  TokenCount reads the number of commas in the pointed nmea_X[] buffer
  pTOKEN = pTOKEN + 1;                  // pTOKEN points advances $/!
  for (int n = 0; n <= Num_DataFields - 1; n++) {
    p = Field[n];
    if (!FillToken(Field[n])) { return false; }  // FillToken looks for "," places the data pointed to into Field[n]
  }
  p = Field[Num_DataFields];  // searches for '*' and if not found, looks for a CR
  if (!FillTokenLast(Field[Num_DataFields])) { return false; }
  //Serial.printf("  Found  <%i> Fields Field0<%s> Field1<%s> Field2<%s> Field3<%s>\n", Num_DataFields, Field[0],Field[1], Field[2], Field[3]);
  //NeedleInHaystack/4/will (should !) identify the command.  Note Nul to prevent zero ! being passed to Switch or Div4
  //                  0   1   2   3   4   5   6   7   8   9   10  11  12  13  14  15  16  17  18  19....
  char nmeafunct[] = "NUL,DBT,DPT,DBK,MWV,VHW,RMC,APB,GLL,HDG,HDM,MTW,MWD,NOP,XS,,AK,,ALK,BWC,WPL,GSV ";  // more can be added to
  // Not using Field[0] as some commands have only two characters. so we can look for (eg) 'XS,' from $IIXS, =13
  if (NeedleinHaystack(buf[3], buf[4], buf[5], nmeafunct, Index) == false) { return false; }
  // Serial.printf(" Using case %i \n", Index / 4);
  // Serial.println(" Fields:");for(int x=0 ;int <Num_DataFields;int++){Serial.print(Field[x]);Serial.print(",");} Serial.println("> ");

  switch (Index / 4) {
    case 1:  //dbt
      toNewStruct(Field[3], BoatData.WaterDepth);
      // BoatData.WaterDepth = atof(Field[3]);
      return true;
      break;
    case 2:  //DPT //dIFFERENT TO DBT/DBK
      toNewStruct(Field[1], BoatData.WaterDepth);
      // BoatData.WaterDepth = atof(Field[1]);
      return true;
      break;
    case 3:  //DBK
      toNewStruct(Field[3], BoatData.WaterDepth);
      //  BoatData.WaterDepth = atof(Field[3]);
      return true;
      break;

    case 4:  //mwv
      toNewStruct(Field[1], BoatData.WindAngle);
      toNewStruct(Field[1], BoatData.WindAngleA);
      toNewStruct(Field[3], BoatData.WindSpeedK);

      return true;
      break;

    case 5:  //VHW
      toNewStruct(Field[5], BoatData.STW);
      // BoatData.STW = atof(Field[5]);  // other VHW data (directions!) are usually false!
      return true;
      break;
    case 6:  //RMC
      toNewStruct(Field[7], BoatData.SOG);
      toNewStruct(Field[8], BoatData.COG);
      // BoatData.SOG = atof(Field[7]);  //  was just atof( or  use TL function NMEA0183GetDouble to cover some error cases and return NMEA0183DoubleNA if N/A
      // BoatData.COG = atof(Field[8]);  // atof sets nmea0183nan (-10million.. so may need extra stuff to prevent silly displays!)
      BoatData.Latitude = LatLonToDouble(Field[3], Field[4][0]);   // using TL's functions
      BoatData.Longitude = LatLonToDouble(Field[5], Field[6][0]);  //nb we use +1 on his numbering that omits the command
                                                                   //        Serial.println(BoatData.GPSTime); Serial.println(BoatData.Latitude);  Serial.println(BoatData.Longitude);  Serial.println(BoatData.SOG);
      BoatData.GPSTime = NMEA0183GPTimeToSeconds(Field[1]);
      BoatData.GPSDate = atof(Field[9]);

      return true;  //
      break;
    case 7:  //APA  3=xte 8 = bearing to dest (9=M agnetic or  T rue)
             //APB  3= xte 11 = CURRENT BEARING TO DEST  and 12(m/t) same..AS APA

      return true;
      break;
    case 10:  //HDM
      toNewStruct(Field[1], BoatData.MagHeading);
      //BoatData.MagHeading = atof(Field[1]);
      return true;
      break;

    case 19:  //GSV
      // Serial.printf("\n Debug GSV ? numdatafields<%i>  ", Num_DataFields);
      //          if (Num_DataFields < 10) { return false; }
      //        Serial.println("Fields<");                  // un copy this lot to assist debugging!!
      //        for (int x = 0; x <= Num_DataFields; x++) {
      //          Serial.printf("%i=<%s>,",x,Field[x]);
      //        }
      //        Serial.println(" ");
      toNewStruct(Field[3], BoatData.SatsInView);
      //BoatData.SatsInView = atof(Field[3]);
      return true;
      break;

    default:
      return false;
      break;
  }


  return false;
}
// chararrayToDouble -- get char*, check it is not null then atof to the double. Use to make a safer atof().

// These are the original conversions - remove correct and delete as we progress.....

// //         if(command == "APB") {
// //  //          Serial.print("APB");    //autopilot
// //         } else if (command == "DBK") {
// //   //          Serial.print("DBK");
// //         } else if (command == "DBT") {
// //           BoatData.WaterDepth = Field[3];
// //           depth = BoatData.WaterDepth;
// //         } else if (command == "DPT") {
// //  /*
// //           BoatData.WaterDepth = Field[1];
// //           depth = BoatData.WaterDepth;
// //           jsonDoc["depth"] = depth;
// //           notifyClients();
// //  */
// //         } else if (command == "GGA") {
// //           BoatData.UTC = Field[1];
// //           int hours = BoatData.UTC.substring(0, 2).toInt();
// //           int minutes = BoatData.UTC.substring(2, 4).toInt();
// //           int seconds = BoatData.UTC.substring(4, 6).toInt();
// //           BoatData.GPSTime = BoatData.UTC.toDouble();
//           BoatData.UTC = int2string(hours) + ":" + int2string(minutes) + ":" + int2string(seconds);
//           timedate = BoatData.UTC;
//           jsonDoc["timedate"] = timedate;
//  //          Serial.printf("GGA %s", timedate);
//           BoatData.Latitude = convertGPString(Field[2]) + Field[3];
//           latitude = BoatData.Latitude;
//           jsonDoc["latitude"] = latitude;
//           BoatData.Longitude = convertGPString(Field[4]) + Field[5];
//           longitude = BoatData.Longitude;
//           jsonDoc["longitude"] = longitude;
//           notifyClients();
//         } else if (command == "GLL") {
//           BoatData.Latitude = convertGPString(Field[1]) + Field[2];
//           latitude = BoatData.Latitude;
//           jsonDoc["latitude"] = latitude;
//           BoatData.Longitude = convertGPString(Field[3]) + Field[4];
//           longitude = BoatData.Longitude;
//           jsonDoc["longitude"] = longitude;
//           BoatData.UTC = Field[5];
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
//           BoatData.MagHeading = String( int(Field[1]););
//           heading = BoatData.MagHeading + "°";
//           jsonDoc["heading"] = heading;
//           notifyClients();
//         } else if (command == "HDM") {
//           BoatData.MagHeading = String( int(Field[1]););
//           heading = BoatData.MagHeading + "°";
//           jsonDoc["heading"] = heading;
//           notifyClients();
//         } else if (command == "HDT") {
//           BoatData.HeadingT = String( int(Field[1]););
//           heading = BoatData.HeadingT + "°";
//           jsonDoc["heading"] = heading;
//           notifyClients();
//         } else if (command == "MTW") {
//           BoatData.WaterTemperature = Field[1];
//           watertemp = BoatData.WaterTemperature + "°C";
//           jsonDoc["watertemp"] = watertemp;
//           notifyClients();
//         } else if (command == "MWD") {
//           BoatData.WindDirectionT = String( int(Field[1]););
//           winddir  = BoatData.WindDirectionT + "°true";
//           jsonDoc["winddir"] = winddir;
//           BoatData.WindDirectionM = String( int(Field[3]););
//   //          winddir  = BoatData.WindDirectionM + "m";
//   //          jsonDoc["winddir"] = winddir;
//           BoatData.WindSpeedK = String( int(Field[5]););
//           windspeed = BoatData.WindSpeedK;
//           jsonDoc["windspeed"] = windspeed;
//           notifyClients();
//         } else if (command == "MWV") { //wind speed and angle
//           BoatData.WindDirectionT = atof(Field[1]);
//           BoatData.WindSpeedK = atof(Field[3]);
//         } else if (command == "RMB") {    //nav info
//   //          Serial.print("RMB");        //waypoint info
//         } else if (command == "RMC") {    //nav info
//           BoatData.SOG = Field[7];
//           speed = BoatData.SOG;
//           jsonDoc["speed"] = speed;
//           BoatData.Latitude = convertGPString(Field[3]) + Field[4];
//           latitude = BoatData.Latitude;
//           jsonDoc["latitude"] = latitude;
//           BoatData.Longitude = convertGPString(Field[5]) + Field[6];
//           longitude = BoatData.Longitude;
//           jsonDoc["longitude"] = longitude;
//           BoatData.UTC = Field[1];
//           int hours = BoatData.UTC.substring(0, 2).toInt();
//           int minutes = BoatData.UTC.substring(2, 4).toInt();
//           int seconds = BoatData.UTC.substring(4, 6).toInt();
//           BoatData.UTC = int2string(hours) + ":" + int2string(minutes) + ":" + int2string(seconds);
//           BoatData.Date = "20"+ Field[9].substring(4,6) + "-" + Field[9].substring(2,4) + "-" + Field[9].substring(0,2) ;
//           timedate = BoatData.Date + " " + BoatData.UTC;
//           jsonDoc["timedate"] = timedate;
//           notifyClients();
//   //          Serial.printf("RMC %s", timedate);
//         } else if (command == "RPM") {  //engine RPM
//           if (Field[2] == "1") {     //engine no.1
//             BoatData.RPM = String( int (Field[3]););
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
//
//           speed = BoatData.Speed;
//           jsonDoc["speed"] = speed;
//           notifyClients();
//         //
//         } else if (command == "VTG") {  //Track Made Good and Ground Speed
//         //
//         } else if (command == "ZDA") { //Date&Time
//           BoatData.Date = Field[4] + "-" + Field[3] + "-" + Field[2];
//           BoatData.UTC = Field[1];
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

int yforLine(Button button, int printline) {
  int y = button.v + button.bordersize;
  y = y + printline * (text_height + 2);
  return y;
}




void UpdateLinef(Button &button, const char *fmt, ...) {  // Types sequential lines in the button space '&' for button to store printline?
  //static int button.PrintLine; // place in button so its static for each button!
  int LinesOfType, characters;
  static bool screenfull;
  char limitedWidth[80];
  int16_t x, y, TBx1, TBy1;
  uint16_t TBw, TBh;
  int typingspaceH, typingspaceW;
  if (screenfull && debugpause) { return; }
  typingspaceH = button.height - (2 * button.bordersize);
  typingspaceW = button.width - (2 * button.bordersize);
  LinesOfType = typingspaceH / (text_height + 2);  //assumes textsize 1?
  static char msg[300] = { '\0' };
  va_list args;
  va_start(args, fmt);
  vsnprintf(msg, 128, fmt, args);
  va_end(args);
  int len = strlen(msg);
  x = button.h + button.bordersize;  // shift inwards for border
  gfx->setTextBound(button.h + button.bordersize, button.v + button.bordersize, typingspaceW, typingspaceH);
  gfx->setTextWrap(true);
  y = yforLine(button, button.PrintLine);  //Needed for the text bounds?
  gfx->getTextBounds(msg, x, y, &TBx1, &TBy1, &TBw, &TBh);
  // if (TBh/text_height) {  // calculate how many lines??
  //   Serial.printf(" x %i y%i &TBx1 %i, &TBy1 %i, &TBw %i, &TBh %i \n",x,y, TBx1, TBy1, TBw, TBh);
  //tbh is always the full height of the box!
  // not needed as clear on full screen is simpler ?
  gfx->fillRect(x, yforLine(button, button.PrintLine), typingspaceW, TBh + 2, button.backcol);

  gfx->setCursor(x, yforLine(button, button.PrintLine) + text_offset + 1);  // puts cursor on a specific line with 2 pixels of V spacing
  //gfx->setTextColor(button.textcol);
  gfx->print(msg);
  //Serial.printf(" lines  tbh %i textheight %i  >lines are %i  \n",TBh,text_height,TBh/text_height);                                                       // lines should be blanked by previous filRect
  button.PrintLine = button.PrintLine + (TBh / (text_height + 2)) + 1;
  if (button.PrintLine >= (LinesOfType)) {
    screenfull = true;
    if (!debugpause) {
      button.PrintLine = 0;
      gfx->fillRect(button.h + button.bordersize, button.v + button.bordersize, typingspaceW, typingspaceH, button.backcol);
      screenfull = false;
    }
  }
  gfx->setTextBound(0, 0, 480, 480);  //MUST RESET IT
}

// simplify call for special instData data types..
////  ******* First a LOCAL function **********
void NEWUpdate(Button button, instData &data, const char *fmt, ...) {  // Centers text in space GREYS if data is OLD
  static char msg[300] = { '\0' };
  //Serial.printf("h %i v %i TEXT %i  Background %i \n",button.h,button.v, button.textcol,button.backcol);
  int textsize = 1;
  // calculate new offsets to just center on original box - minimum redraw of blank
  int16_t x, y, TBx1, TBy1;
  uint16_t TBw, TBh;
  int typingspaceH, typingspaceW;
  bool recent = (data.updated >= millis() - 3000);
  typingspaceH = button.height - (2 * button.bordersize);
  typingspaceW = button.width - (2 * button.bordersize);
  gfx->setTextSize(textsize);  // used in message buildup
  va_list args;
  va_start(args, fmt);
  vsnprintf(msg, 128, fmt, args);
  va_end(args);
  int len = strlen(msg);
  gfx->setTextBound(button.h + button.bordersize, button.v + button.bordersize, typingspaceW, typingspaceH);
  gfx->getTextBounds(msg, button.h, button.v, &TBx1, &TBy1, &TBw, &TBh);  // do not forget '& ! Pointer not value!!!
  x = button.h + button.bordersize;
  y = button.v + button.bordersize + (text_offset * textsize);
  x = x + ((button.width - TBw - (2 * button.bordersize)) / 2) / textsize;                     //try horizontal centering
  y = y + ((button.height - TBh - (2 * button.bordersize)) / 2) / textsize;                    //try vertical centering
  gfx->fillRect(x - 8, y - (text_offset * textsize) - 4, TBw + 16, TBh + 10, button.backcol);  // visualise by changing colour ! where the text will be plus a bit
  gfx->setTextColor(button.textcol);
  gfx->setCursor(x, y);
  if (!recent) {
    gfx->setTextColor(DARKGREY);
    data.greyed = true;
  }
  gfx->print(msg);
  gfx->setTextSize(1);
  gfx->setTextColor(button.textcol);
  gfx->setTextBound(0, 0, 480, 480);  //MUST reset it !
  data.displayed = true;              //reset to false inside toNewStruct
}
//******* Now the generic version used in the display code will return instantly if data is not recent and has already been displayed
void UpdateData(Button button, instData &data, const char *fmt) {

  bool recent = (data.updated >= millis() - 3000);
  if (data.greyed) { return; }
  if (!data.displayed) { NEWUpdate(button, data, fmt, data.data); }
  if (!recent && !data.greyed) { NEWUpdate(button, data, fmt, data.data); }
}


void UpdateCentered(Button button, const char *fmt, ...) {  // Centers text in space
  static char msg[300] = { '\0' };
  //Serial.printf("h %i v %i TEXT %i  Background %i \n",button.h,button.v, button.textcol,button.backcol);
  int textsize = 1;
  // calculate new offsets to just center on original box - minimum redraw of blank
  int16_t x, y, TBx1, TBy1;
  uint16_t TBw, TBh;
  int typingspaceH, typingspaceW;
  typingspaceH = button.height - (2 * button.bordersize);
  typingspaceW = button.width - (2 * button.bordersize);
  gfx->setTextSize(textsize);  // used in message buildup
  va_list args;
  va_start(args, fmt);
  vsnprintf(msg, 128, fmt, args);
  va_end(args);
  int len = strlen(msg);
  gfx->setTextBound(button.h + button.bordersize, button.v + button.bordersize, typingspaceW, typingspaceH);
  gfx->getTextBounds(msg, button.h, button.v, &TBx1, &TBy1, &TBw, &TBh);  // do not forget '& ! Pointer not value!!!
  gfx->fillRect(TBx1, TBy1, TBw, TBh, LIGHTGREY);                         // visualise
  x = button.h + button.bordersize;
  y = button.v + button.bordersize + (text_offset * textsize);
  x = x + ((button.width - TBw - (2 * button.bordersize)) / 2) / textsize;   //try horizontal centering
  y = y + ((button.height - TBh - (2 * button.bordersize)) / 2) / textsize;  //try vertical centering
  //gfx->fillRect(button.h + button.bordersize, y - TBh - 1, button.width - (2 * button.bordersize), TBh + 4, button.backcol);  // just (plus a little bit ) where the text will be
  gfx->setTextColor(button.textcol);
  gfx->setCursor(x, y);
  gfx->print(msg);
  gfx->setTextSize(1);
  gfx->setTextBound(0, 0, 480, 480);  //MUST reset it !
}

void UpdateCentered(int h, int v, int width, int height, int bordersize,
                    uint16_t backgroundcol, uint16_t textcol, uint16_t BorderColor, const char *fmt, ...) {  //Print in a box.(h,v,width,height,textsize,bordersize,backgroundcol,textcol,BorderColor, const char* fmt, ...)
  static char msg[300] = { '\0' };
  // calculate new offsets to just center on original box - minimum redraw of blank
  int16_t x, y, TBx1, TBy1;
  uint16_t TBw, TBh;
  va_list args;
  va_start(args, fmt);
  vsnprintf(msg, 128, fmt, args);
  va_end(args);
  int len = strlen(msg);
  gfx->getTextBounds(msg, h, v, &TBx1, &TBy1, &TBw, &TBh);  // do not forget '& ! Pointer not value!!!
  x = h + bordersize;
  y = v + bordersize + (text_offset);
  x = x + ((width - TBw - (2 * bordersize)) / 2);   //try horizontal centering
  y = y + ((height - TBh - (2 * bordersize)) / 2);  //try vertical centering
                                                    // gfx->fillRect(h + bordersize, v + bordersize, width - (2 * bordersize), height - (2 * bordersize), backgroundcol);  // full inner rectangle blanking
  gfx->fillRect(h + bordersize, y - TBh - 1, width - (2 * bordersize), TBh + 4, backgroundcol);
  gfx->setTextColor(textcol);
  gfx->setCursor(x, y);
  gfx->print(msg);
  gfx->setTextSize(1);
}

//USED BY GFXBorderBoxPrintf
void WriteinBorderBox(int h, int v, int width, int height, int bordersize,
                      uint16_t backgroundcol, uint16_t textcol, uint16_t BorderColor, const char *TEXT) {  //Write text in filled box of text height at h,v (using fontoffset to use TOP LEFT of text convention)
  int16_t x, y, TBx1, TBy1;
  uint16_t TBw, TBh;
  // gfx->setTextSize(textsize);
  x = h + bordersize;
  y = v + bordersize + (text_offset);                        //initial Top Left positioning for print text
  gfx->getTextBounds(TEXT, x, y, &TBx1, &TBy1, &TBw, &TBh);  // do not forget '& ! Pointer not value!!!
  //Serial.printf(" Text bounds planned print (x%i y%i)  W:%i H:%i TB_X %i  TB_Y %i \n",x,y, TBw,TBh,TBx1,TBy1);
  //gfx->fillRect(TBx1 , TBy1 , TBw , TBh , WHITE); delay(100); // visulize what the data is!
  // move to center is  add (width-2*bordersize-TBw)/2 ?
  //move vertical is add (height -2*bordersize-TBh)/2
  gfx->fillRect(h, v, width, height, BorderColor);  // width and height are for the OVERALL box.
  gfx->fillRect(h + bordersize, v + bordersize, width - (2 * bordersize), height - (2 * bordersize), backgroundcol);
  gfx->setTextColor(textcol);

  // offset up/down by OFFSET (!) for GFXFONTS that start at Bottom left. Standard fonts start at TOP LEFT
  x = h + bordersize;
  y = v + bordersize + (text_offset);
  x = x + ((width - TBw - (2 * bordersize)) / 2);   //try horizontal centering
  y = y + ((height - TBh - (2 * bordersize)) / 2);  //try vertical centering
  gfx->setCursor(x, y);
  gfx->print(TEXT);
  //reset font ...
  gfx->setTextSize(1);
}

void GFXBorderBoxPrintf(int h, int v, int width, int height, int bordersize,
                        uint16_t backgroundcol, uint16_t textcol, uint16_t BorderColor, const char *fmt, ...) {  //Print in a box.(h,v,width,height,textsize,bordersize,backgroundcol,textcol,BorderColor, const char* fmt, ...)
  static char msg[300] = { '\0' };                                                                               // used in message buildup
  va_list args;
  va_start(args, fmt);
  vsnprintf(msg, 128, fmt, args);
  va_end(args);
  int len = strlen(msg);
  WriteinBorderBox(h, v, width, height, bordersize, backgroundcol, textcol, BorderColor, msg);
}

void GFXBorderBoxPrintf(Button button, const char *fmt, ...) {
  static char msg[300] = { '\0' };  // used in message buildup
  va_list args;
  va_start(args, fmt);
  vsnprintf(msg, 128, fmt, args);
  va_end(args);
  int len = strlen(msg);
  WriteinBorderBox(button.h, button.v, button.width, button.height, button.bordersize, button.backcol, button.textcol, button.BorderColor, msg);
}

void AddTitleBorderBox(Button button, const char *fmt, ...) {  // add a title to the box
  int Font_Before;
  //Serial.println("Font at start is %i",MasterFont);
  Font_Before = MasterFont;
  setFont(0);
  static char Title[300] = { '\0' };
  int16_t x, y, TBx1, TBy1;
  uint16_t TBw, TBh;  // used in message buildup
  va_list args;
  va_start(args, fmt);
  vsnprintf(Title, 128, fmt, args);
  va_end(args);
  int len = strlen(Title);
  gfx->getTextBounds(Title, button.h, button.v, &TBx1, &TBy1, &TBw, &TBh);
  gfx->setTextColor(WHITE, button.BorderColor);
  if ((button.v - TBh) >= 0) {
    gfx->setCursor(button.h, button.v);
  } else {
    gfx->setCursor(button.h, button.v + TBh);
  }
  gfx->print(Title);
  setFont(Font_Before);  //Serial.println("Font selected is %i",MasterFont);
}

void AddTitleBorderBox(int h, int v, uint16_t BorderColor, const char *fmt, ...) {  // add a title to the box
  int Font_Before;
  //Serial.println("Font at start is %i",MasterFont);
  Font_Before = MasterFont;
  setFont(0);
  static char Title[300] = { '\0' };
  int16_t x, y, TBx1, TBy1;
  uint16_t TBw, TBh;  // used in message buildup
  va_list args;
  va_start(args, fmt);
  vsnprintf(Title, 128, fmt, args);
  va_end(args);
  int len = strlen(Title);
  gfx->getTextBounds(Title, h, v, &TBx1, &TBy1, &TBw, &TBh);
  gfx->setTextColor(WHITE, BorderColor);
  if ((v - TBh) >= 0) {
    gfx->setCursor(h, v);
  } else {
    gfx->setCursor(h, v + TBh);
  }
  gfx->print(Title);
  setFont(Font_Before);  //Serial.println("Font selected is %i",MasterFont);
}

int Circular(int x, int min, int max) {  // returns circulated data in range min to max
  // based on compass idea in sincos: Normalize the input  to the range min (0) to max (359)
  while (x < min)
    x += max + 1;
  while (x > max)
    x -= max + 1;
  return x;
}

int GraphRange(double data, int low, int high, double dmin, double dmax) {  // returns int (range low to high) proportionate to position of data relative to min / max
 // Serial.printf(" debug GraphRange data%3.2f low%i high%i dmin%3.2f dmax%3.2f ",data,low,high,dmin,dmax);
  int graphpoint;
  double input_ratio = (data - dmin) / (dmax - dmin);
  //Serial.printf("   GraphRange  percent is %3.2f  based on data%3.2f dmin%3.2f dmax%3.2f \n",input_ratio, dmin,dmax);
  graphpoint = int(low + (input_ratio * (high - low)));
 // Serial.printf("  results in graph point %i   \n",graphpoint);
  return graphpoint;
}

void PTriangleFill(Phv P1, Phv P2, Phv P3, uint16_t COLOUR) {
  gfx->fillTriangle(P1.h, P1.v, P2.h, P2.v, P3.h, P3.v, COLOUR);
}
void Pdrawline(Phv P1, Phv P2, uint16_t COLOUR) {
  gfx->drawLine(P1.h, P1.v, P2.h, P2.v, COLOUR);
}
void PfillCircle(Phv P1, int rad, uint16_t COLOUR) {
  gfx->fillCircle(P1.h, P1.v, rad, COLOUR);
}





void DrawGraph(bool reset, Button button, double data, double dmin, double dmax) {
  static Phv graph[102];
  static int index;
  int dotsize;
  dotsize=4;
  if (reset) {
    for (int x = 0; x <= 100; x++) {//set all mid point 
       graph[x].v =  (button.v + button.bordersize+ (button.height/2));
      graph[x].h = (button.h + button.bordersize+ (button.width/2));
    }
   // Serial.printf(" Debug Graph..'zeroed' to mid point of  h%i  v%i",graph[1].h,graph[1].v);
    //initial fill and border (same as write in border box codes )
    gfx->fillRect(button.h, button.v, button.width, button.height, button.BorderColor);  // width and height are for the OVERALL box.
    gfx->fillRect(button.h + button.bordersize, button.v + button.bordersize,
                  button.width - (2 * button.bordersize), button.height - (2 * button.bordersize), button.backcol);
    index = 0;
    return;
  }
 // Serial.printf(" Debug Graph data %3.2f  dmin %3.2f dmax %3.2f \n",data,dmin,dmax);
  if (data >= dmax) { data = dmax; }
  if (data <= dmin) { data = dmin; }
  //Serial.printf(" Debug Graph erasing old point index %i..  plot.h:%i  and plot.v%i\n", index, graph[index].h, graph[index].v);
  PfillCircle(graph[index], dotsize, button.backcol);
  Pdrawline(graph[index],graph[index+1],button.backcol);//rease line in front of the one to draw now
  graph[index].v = GraphRange(data, button.v + button.height - button.bordersize-dotsize, button.v + button.bordersize+dotsize, dmin, dmax);
  graph[index].h = GraphRange(index, button.h + button.bordersize+dotsize, button.h + button.width - button.bordersize-dotsize, 0, 100);
  //Serial.printf(" Debug Graph new data index %i.. data %f becomes ploth:%i  become plotv%i\n", index, data, graph[index].h, graph[index].v);
  if (index > 0) {Pdrawline(graph[index-1],graph[index],button.textcol);}
  PfillCircle(graph[index], 4, button.textcol);
  index = Circular(index + 1, 0, 100);  //circular increment but not to zero 
  //Serial.printf(" PfillCircle  plotted \n");
  
  // for (int x = 0; x <= 100; x++) {  //draw line from index-1 to index in text col
  //                                   // Pdrawline(graph[x], graph[x + 1], button.textcol);
  // }
}
